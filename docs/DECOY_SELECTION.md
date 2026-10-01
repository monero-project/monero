# Implementing Monero Decoy Selection

## Don't be Unique!

We don't recommend that you implement decoy selection yourself, due to the numerous examples of decoy selection algorithms
which didn't match the exact behavior of the reference implementation in `wallet2`. When this happens, chain analysis
tools can be used identify the software used to create those transactions, which reduces overall anonymity. If you are
able to do so, try to first use an existing implementation of decoy selection before rolling your own. There is a Python
script in the core repository (utils/python-rpc/decoy_selection.py) that provides an API for doing RingCT decoy selection.
You can also run the script as main and generate a TXT file containing as many decoy selections as you desire. Additionally,
this document contains a line-by-line guide for that reference code. However, if you are going to do roll your own,
please take great care to make your implementation matches the exact behavior of the reference code. If you're just here
to see how it works, welcome! Let's get into it...

## Background Information

### How Transaction Outputs are Represented

In Bitcoin, when we want to spend an ["Unspent Transaction Output" (UTXO)](https://en.wikipedia.org/wiki/Unspent_transaction_output),
we reference that UTXO with two pieces of information: a [transaction ID](https://wiki.bitcoinsv.io/index.php/TXID), and
an [output index](https://developer.bitcoin.org/terms.html#term-output-index). The transaction ID tells us in which
transaction the UTXO was created, and the output index is an index for outputs within that transaction.

However, in Monero, we use two different pieces of information to reference transaction outputs inside of transaction
inputs: the output amount, and a global output index. Since all information on the blockchain can be strictly ordered
sequentially (this block before that block, this transaction before that transaction, this output before that output,
etc), we can assign integer indexes for every transaction output on the chain with a given amount. This is what the
"global output index" is. The output amount is simply the number of piconero (the smallest denomination of Monero coin)
publicly assigned to that transaction output, and 0 if the transaction output is a RingCT output (that is, the amount is
hidden). The reason that we differentiate between outputs of different amounts is that any one ring signature can only
contain ring members with the same amount.

Finally, when we are doing decoy selection to find the other members of a ring, our result is a list of global output
indexes, which represent a set of transaction outputs with the same amount as the transaction output we are trying to
spend. We sample these global output indices according to a certain distribution, with this distribution hopefully
statistically matching the distribution of the ages of "true spends", so that the ring member we truly wish to spend is
masked from external observers within a certain probability.

### How Transaction Unlock Times Affect Decoy Selection

To get a basic understanding of transaction unlock times, you can read [this blog post](https://www.getmonero.org/resources/moneropedia/unlocktime.html).
As it pertains to implementing decoy selection, there are two main wrinkles that one must contend
with: the fact that arbitrary values inside the unlock time field negatively impact uniformity, and
the fact that one has to assert that all potential decoy outputs for a given ring are unlocked. Uniformity
issues are outside of the scope of this document, but if you want to learn more about that, TheCharlatan
has done a lot of research on this specific topic which you can find [here](https://thecharlatan.ch/Monero-Unlock-Time-Privacy/).
For now, let us focus on the second issue: making sure that all of our decoys are unlocked.

As of the time of this writing, there is no way to fetch specifically only locked outputs without iterating through all
outputs on-chain. However, there aren't too many transactions with custom unlock times, only a fraction of a percent, so
the tactic that the core wallet takes is to request way more outputs per ring than needed, assuming that some will be 
unusable. It then picks from the valid remaining outputs. The trap that one can fall into when doing this is increasing
statistical dependence for picks within rings more than necessary. When you are trying to build up a set of X unique
decoy picks, if the first pick has 100 choices, then the next pick has 99 choices, then 98 choices, etc, etc. Since
these picks are not statistically independent, then the distribution of the picks gets more and more skewed for the later
picks. You can combat this effect by simply committing to the order in which you pick the outputs, and try adding them
in this order, assuming that they are valid.

## Implementing Decoy Selection

This document was updated in October 2023, and is providing instructions for how to implement Monero decoy selection
according to the code in the core repository as of commit 67d190c. This guide does not mention segregation fork heights,
the ring database, or other key image reuse mitigation features, which are used in the core wallet code. If you would
like to see how those extra features are implemented in practice, take a look at the higher-level decoy selection method
[`tools::wallet2::get_outs()`](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L8527).
This guide also focuses mainly on decoy selection for RingCT enotes. Pre-RingCT decoy selection uses different distributions
and methods.

### First, Some Numeric Constants

* `GAMMA_SHAPE = 19.28` [source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L141-L142)
  * Shape parameter for a [gamma distribution](https://en.wikipedia.org/wiki/Gamma_distribution)
* `GAMMA_RATE = 1.61` [source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L141-L142)
  * Rate parameter for a [gamma distribution](https://en.wikipedia.org/wiki/Gamma_distribution)
  * :memo: **NOTE**: Here we used a "rate" parameter, but gamma distributions can also be parameterized with a "scale" parameter, where `scale = 1 / rate`. If you use a library to sample from a gamma distribution, make sure you don't get rate & scale mixed up.
* `DIFFICULTY_TARGET_V2 = 120` [source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/cryptonote_config.h#L79)
  * The current protocol target blocktime, in seconds
* `CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE = 10` [source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/cryptonote_config.h#L48)
  * The minimum number of blocks between a transaction output's creation height and the first block height than it can be used as a ring member for a transaction input.
* `DEFAULT_UNLOCK_TIME = CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE * DIFFICULTY_TARGET_V2 = 1200` [source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L151-L152)
  * The represents `CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE` converted into an average number of seconds. In other words, it is the average time period from an output's creation to when it can be spent. 
* `RECENT_SPEND_WINDOW = 15 * DIFFICULTY_TARGET_V2 = 1800` [source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L151-L152)
  * This value is an arbitrary short window in which it is assumed that a large portion of transaction outputs will be attempted to be spent. The justication for picking 30 minutes is the emperical analysis of the Litecoin chain (source needed).
* `SECONDS_IN_A_YEAR = 60 * 60 * 24 * 365 = 31536000` [source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L1035)
  * Self-explanatory. Even if not technically correct, do not use a different value.
* `BLOCKS_IN_A_YEAR = SECONDS_IN_A_YEAR / DIFFICULTY_TARGET_V2 = 262800` [source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L1035)
  * The average number of expected blocks to be mined each year

### The Cumulative RingCT Outputs-per-block Distribution, or *CROD*

The only data structure we need (at least for RingCT decoy selection) during runtime is called the "Cumulative RingCT
Outputs-per-block Distribution" (*CROD*). This is simply a big list of the cumulative number of RingCT transaction
outputs at each block, inclusive, starting with the very first RingCT block. To match most programming languages, we
index from 0. For reference, the RingCT hardfork height on the Monero mainnet is 1220516, and you can find more
information about the hardforks in [this C++ file](https://github.com/monero-project/monero/blob/master/src/hardforks/hardforks.cpp).
So for example, the *CROD* for the Monero mainnet will have values:

```
CROD = 1, 12, 12, 23, 23, 34, 45, 57, 66, 78, 90, ...

CROD[0]  =  1    # representing height 1220516, which itself has  1 RCT outputs
CROD[1]  = 12    # representing height 1220517, which itself has 11 RCT outputs
CROD[2]  = 12    # representing height 1220518, which itself has  0 RCT outputs
CROD[3]  = 23    # representing height 1220519, which itself has 11 RCT outputs
CROD[4]  = 23    # representing height 1220520, which itself has  0 RCT outputs
CROD[5]  = 34    # representing height 1220521, which itself has 11 RCT outputs
CROD[6]  = 45    # representing height 1220522, which itself has 11 RCT outputs
CROD[7]  = 57    # representing height 1220523, which itself has 12 RCT outputs
CROD[8]  = 66    # representing height 1220524, which itself has  9 RCT outputs
CROD[9]  = 78    # representing height 1220525, which itself has 12 RCT outputs
CROD[10] = 90    # representing height 1220526, which itself has 12 RCT outputs
```

Assuming your `monerod` instance is running on your current machine on port 18081, you can double check this by using the `get_output_distribution` [RPC command](https://www.getmonero.org/resources/developer-guides/daemon-rpc.html#get_output_distribution)
like so (note that `cumulative=true`):

```
$ curl 127.0.0.1:18081/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"get_output_distribution","params":{"amounts":[0],"from_height":1220516,"to_height":1220526,"binary":false,"cumulative":true}}' -H 'Content-Type: application/json'
{
  "id": "0",
  "jsonrpc": "2.0",
  "result": {
    "credits": 0,
    "distributions": [{
      "amount": 0,
      "base": 0,
      "binary": false,
      "compress": false,
      "distribution": [1,12,12,23,23,34,45,57,66,78,90]
      "start_height": 1220516
    }],
    "status": "OK",
    "top_hash": "",
    "untrusted": false
  }
}
```

The final element of *CROD* should represent the total number of RingCT transaction outputs for the entire entire chain,
up to and including the latest known block. In this guide we use the name `CROD_length` as a label for the number of
elements in the *CROD*. 

### *CROD* Setup

Everytime the *CROD* changes, we need to set two variables: `average_output_delay` and `num_usable_rct_outputs`.
`average_output_delay` is meant to quantify the average number of seconds between new transaction outputs over the last
year or so of chain data. `num_usable_rct_outputs` is the total number of on-chain RingCT outputs that *will be*
(not currently) at least 10 blocks old when the next block is mined. Non-coinbase outputs of this age will not be locked
due to the default 10-block-lock concensus rule. :memo: **Note**: Just because outputs are at least 10 blocks old does
not guaranteed that they can be unlocked; coinbase outputs are
[locked for 60 blocks](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/cryptonote_config.h#L43),
and any transaction can specify [any arbitrary additional lock time](https://www.getmonero.org/resources/moneropedia/unlocktime.html).

#### How to calculate `average_output_delay`

1. Count the number of blocks in a year, or the number of blocks in *CROD*, whichever is fewer
    * `num_blocks_to_consider_for_delay = min(CROD_length, BLOCKS_IN_A_YEAR)`
2. Count the number of outputs within the newest `num_blocks_to_consider_for_delay` blocks, including the top block:
    - If `CROD_length > num_blocks_to_consider_for_delay` (this should always be the case a year after the RingCT fork):
      * `num_outputs_to_consider_for_delay = CROD[CROD_length - 1] - CROD[CROD_length - 1 - num_blocks_to_consider_for_delay]`
    - Otherwise the oldest RingCT output is less than a year old:
      * `num_outputs_to_consider_for_delay = CROD[CROD_length - 1]`
3. Calculate the average number of seconds between RingCT outputs for the last year
    * `average_output_delay = DIFFICULTY_TARGET_V2 * num_blocks_to_consider_for_delay / num_outputs_to_consider_for_delay`
    * :memo: **Note**: This is the number of seconds per output, *not* the number of outputs per second

[source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L1033-L1042)

#### How to calculate `num_usable_rct_outputs`

1. Count the number of blocks in the *CROD*, excluding the top locked blocks **minus one**, since those outputs are too young
    * `num_usable_crod_blocks = CROD_length - (CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE - 1)`
2. Get the number of outputs in the oldest `num_usable_crod_blocks` in the *CROD*
    * `num_usable_rct_outputs = CROD[num_usable_crod_blocks - 1]`

[source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L1038-L1040)

| :warning: WARNING |
|-------------------|
| We subtract one from the default transaction output unlock time during the `num_usable_crod_blocks` calculation, since we are ostensibly constructing transactions for the *next* block, which is not yet mined. If we pick decoys which are exactly one block too young *right now*, when our transaction is mined, we will maybe pick decoys which are exactly old enough to be spent. However, if we never pick decoys from the oldest block that is currently too young, then our transactions will never contain decoys from the youngest valid block, and we therefore deterministicly leak the "true spend" for a ring that has one youngest possible ring member. This same [off-by-one bug](https://github.com/monero-project/monero/issues/8872) was present for all publicly known decoy selection algorithms for years, only to be patched in April 2023. |

#### Possible Implementation In Python3

```Python
CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE = 10
DIFFICULTY_TARGET_V2 = 120

SECONDS_IN_A_YEAR =  60 * 60 * 24 * 365
BLOCKS_IN_A_YEAR = SECONDS_IN_A_YEAR // DIFFICULTY_TARGET_V2

def calculate_average_output_delay(crod):
    # 1
    num_blocks_to_consider_for_delay = min(len(crod), BLOCKS_IN_A_YEAR)

    # 2
    if len(crod) > num_blocks_to_consider_for_delay:
        num_outputs_to_consider_for_delay = crod[-1] - crod[-(num_blocks_to_consider_for_delay + 1)]
    else:
        num_outputs_to_consider_for_delay = crod[-1]
    
    # 3
    average_output_delay = DIFFICULTY_TARGET_V2 * num_blocks_to_consider_for_delay / num_outputs_to_consider_for_delay

    return average_output_delay

def calculate_num_usable_rct_outputs(crod):
    # 1
    num_usable_crod_blocks = len(crod) - (CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE - 1)

    # 2
    num_usable_rct_outputs = crod[num_usable_crod_blocks - 1]

    return num_usable_rct_outputs
```

### The Gamma Pick

The operation that we are going to call "gamma pick" is the building block for how we do decoy selection in Monero. The
result of this operation is a singular random global output index. Later on, we will iterate the gamma pick operation
until we have built up a set of global output indices of a certain desired size. Here is how to do a single gamma pick:

1. Randomly sample from [gamma distribution](https://en.wikipedia.org/wiki/Gamma_distribution) X ~ &#x393;(&#x3b1;, &#x3b2;), where shape &#x3b1; = `GAMMA_SHAPE` = 19.28 and rate &#x3b2; = `GAMMA_RATE` = 1.61
    * `X ~ Gamma(GAMMA_SHAPE, GAMMA_RATE)`
    * :warning: **WARNING**: As mentioned before, some people parameterize the gamma function using rate, while others use scale. Make sure to look up this detail so you don't get mixed up. For example, the C++ standard library `std::gamma_distribution` is parameterized according to scale &#x3B8; = 1 / &#x3b2;, **<u>even though they use the greek letter &#x3b2; in the documentation</u>**. Failing to get this straight will result in statistically fingerprintable decoy selection algorithm.
2. Random value `X` represents the natural logarithm of the number of seconds before the present time. So let's exponentiate it against *e*:
    * `target_output_age = e^X` (here we use `^` to mean "to the power of")
3. Next, we want to transform this age value into an amount of time that an output has been unlocked. If the target output age is less than the default unlock time, we uniformly randomly pick a duration in the "recent spend window".
    * If `target_output_age > DEFAULT_UNLOCK_TIME`, then `target_post_unlock_output_age = target_output_age - DEFAULT_UNLOCK_TIME`, else `target_post_unlock_output_age ~ U(0, RECENT_SPEND_WINDOW - 1)`, where `U(a, b)` is a [discrete uniform distribution](https://en.wikipedia.org/wiki/Discrete_uniform_distribution) and `n = b - a + 1`
4. Since we have a time-based index for our pick, we need to somehow convert this time-based index into an integer-based output index. We do this by dividing the age since the default unlock time by the average number of outputs per second.
    * `target_num_outputs_post_unlock = floor(target_post_unlock_output_age / average_output_delay)`
5. Here is the first point in which a gamma pick can fail: if the target output index post-unlock is greater than the number of usable outputs on chain:
    * If `target_num_outputs_post_unlock >= num_usable_rct_outputs`, then restart the gamma pick operation from step 1.
6. Now we get what I call a "pseudo global output index". This value *could* be used as a global output index, but since we want all outputs within the same block to have the same chance of being picked, we instead use this global output index to "pick" a block.
    * `pseudo_global_output_index = num_usable_rct_outputs - 1 - target_num_outputs_post_unlock`
7. Let us get the block index containing our pseudo global output index. In practice, one can use a binary search to perform this operation in `O(log(CROD_length))` time.
    * `picked_block_index = i such that CROD[i] <= pseudo_global_output_index < CROD[i + 1]`
8. To pick an output from this block, we need the first global output index in this block.
    * If `picked_block_index == 0`, then `block_first_global_output_index = 0`, else `block_first_global_output_index = CROD[picked_block_index - 1]`
9. From this block, we also need to know how many outputs are contained within:
    * `block_num_outputs = CROD[picked_block_index] - block_first_global_output_index`
10. If there aren't any outputs in this block, then restart the gamma pick operation from step 1.
11. Finally, we uniformly randomly pick an output from said block:
    * `global_output_index_result ~ U(block_first_global_output_index, CROD[picked_block_index] - 1)`

[source](https://github.com/monero-project/monero/blob/67d190ce7c33602b6a3b804f633ee1ddb7fbb4a1/src/wallet/wallet2.cpp#L1049-L1093)

#### Possible Implementation In Python3

```Python
import bisect
import math
import random

GAMMA_SHAPE = 19.28
GAMMA_RATE = 1.61
GAMMA_SCALE = 1 / GAMMA_RATE

CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE = 10
DIFFICULTY_TARGET_V2 = 120
DEFAULT_UNLOCK_TIME = CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE * DIFFICULTY_TARGET_V2
RECENT_SPEND_WINDOW = 15 * DIFFICULTY_TARGET_V2

def gamma_pick(crod, average_output_delay, num_usable_rct_outputs):
    while True:
        # 1
        x = random.gammavariate(GAMMA_SHAPE, GAMMA_SCALE) # parameterized by scale, not rate!

        # 2
        target_output_age = math.exp(x)

        # 3
        if target_output_age > DEFAULT_UNLOCK_TIME:
            target_post_unlock_output_age = target_output_age - DEFAULT_UNLOCK_TIME
        else:
            target_post_unlock_output_age = np.floor(random.uniform(0.0, RECENT_SPEND_WINDOW))

        # 4
        target_num_outputs_post_unlock = int(target_post_unlock_output_age / average_output_delay)

        # 5
        if target_num_outputs_post_unlock >= num_usable_rct_outputs:
            continue

        # 6
        pseudo_global_output_index = num_usable_rct_outputs - 1 - target_num_outputs_post_unlock

        # 7
        picked_block_index = bisect.bisect_left(crod, pseudo_global_output_index)

        # 8
        if picked_block_index == 0:
            block_first_global_out_index = 0
        else:
            block_first_global_output_index = crod[picked_block_index - 1]

        # 9
        block_num_outputs = crod[picked_block_index] - block_first_global_output_index

        # 10
        if block_num_outputs == 0:
            continue

        # 11
        global_output_index_result = int(random.uniform(block_first_global_output_index, crod[picked_block_index]))

        return global_output_index_result
```

## Testing An Implementation

Assuming that we can correctly treat the distribution of picks by the decoy selection process as a continuous
distribution, we can use a two-sample [Kolmogorov–Smirnov Test](https://en.wikipedia.org/wiki/Kolmogorov%E2%80%93Smirnov_test)
to statistically test if a given implementation statistically matches the reference implementation. Running the provided
Python decoy selection reference script (utils/python-rpc/decoy_selection.py) will generate a TXT file containing
decoy selection picks (you can specify how many) separated by newlines. This data can be imported and used to perform
a two-sample KS test using, for example, `scipy.stats.kstest`. Just make sure that when you're testing, you use the same
*CROD* list, which can be enforced in the Python script with the argument `--to-height`. 

There are also bugs that might not show up on statistical tests, but can still be devasting for anonymity, for example
the [10-block-old decoy selection bug](https://github.com/monero-project/monero/issues/8872). Unfortunately, these are
much harder to uncover and can span over any behavior. It is always a good idea to get second opinions when it comes to
implementing subtle code such as decoy selection. If you would like help reviewing your implementation, there will likely
be someone willing to help on the `#Monero-Dev` channel on Matrix.
