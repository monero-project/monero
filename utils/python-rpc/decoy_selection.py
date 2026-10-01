#!/usr/bin/python3

''' See step-by-step accompaniment in docs/DECOY_SELECTION.md '''

import argparse
import bisect
import math
import os
import random
import sys

import framework.daemon

##### Section: "First, Some Numeric Constants" #####
GAMMA_SHAPE = 19.28
GAMMA_RATE = 1.61
GAMMA_SCALE = 1 / GAMMA_RATE

CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE = 10
DIFFICULTY_TARGET_V2 = 120
DEFAULT_UNLOCK_TIME = CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE * DIFFICULTY_TARGET_V2
RECENT_SPEND_WINDOW = 15 * DIFFICULTY_TARGET_V2

SECONDS_IN_A_YEAR =  60 * 60 * 24 * 365
BLOCKS_IN_A_YEAR = SECONDS_IN_A_YEAR // DIFFICULTY_TARGET_V2

##### Section: "How to calculate `average_output_delay`" #####
def calculate_average_output_delay(crod):
    # 1
    num_blocks_to_consider_for_delay = min(len(crod), BLOCKS_IN_A_YEAR)

    # 2
    if len(crod) > num_blocks_to_consider_for_delay:
        num_outputs_to_consider_for_delay = crod[-1] - crod[-(num_blocks_to_consider_for_delay + 1)]
    else:
        num_outputs_to_consider_for_delay = crod[-1]
    
    # 3
    average_output_delay = DIFFICULTY_TARGET_V2 * num_blocks_to_consider_for_delay \
        / num_outputs_to_consider_for_delay

    return average_output_delay

##### Section: "How to calculate `num_usable_rct_outputs`" #####
def calculate_num_usable_rct_outputs(crod):
    # 1
    num_usable_crod_blocks = len(crod) - (CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE - 1)

    # 2
    num_usable_rct_outputs = crod[num_usable_crod_blocks - 1]

    return num_usable_rct_outputs

##### Section: "The Gamma Pick" #####
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
            target_post_unlock_output_age = math.floor(random.uniform(0.0, RECENT_SPEND_WINDOW))

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
        global_output_index_result = int(random.uniform(block_first_global_output_index,
            crod[picked_block_index]))

        return global_output_index_result

def gamma_pick_n_unlocked(num_picks, crod, get_is_outputs_unlocked):
    # This is the maximum number of outputs we can fetch in one restricted RPC request
    # Line 67 of src/rpc/core_rpc_server.cpp in commit ac02af92
    MAX_GETS_OUTS_COUNT = 5000

    # Calculate average_output_delay & num_usable_rct_outputs for given CROD
    average_output_delay = calculate_average_output_delay(crod)
    num_usable_rct_outputs = calculate_num_usable_rct_outputs(crod)

    # Maps RingCT global output index -> whether that output is unlocked at this moment
    # This saves a ton of time for huge numbers of picks
    is_unlocked_cache = {}

    # Potential picks to written, # of potential picks of unknown lockedness, and total # picked
    buffered_picks = []
    num_picks_unknown_locked = 0
    num_total_picked = 0

    # Main picking / RPC loop
    while num_total_picked < num_picks:
        # Do gamma pick
        new_pick = gamma_pick(crod, average_output_delay, num_usable_rct_outputs)
        buffered_picks.append(new_pick)
        num_picks_unknown_locked += int(new_pick not in is_unlocked_cache)

        # Once num_picks_unknown_locked or buffered_picks is large enough, trigger "flush"...
        should_flush = num_picks_unknown_locked == MAX_GETS_OUTS_COUNT or \
                num_total_picked + len(buffered_picks) == num_picks
        if should_flush:
            # Update is_unlocked_cache if # outputs w/ unknown locked status is non-zero
            unknown_locked_outputs = [o for o in buffered_picks if o not in is_unlocked_cache]
            assert len(unknown_locked_outputs) == num_picks_unknown_locked
            if unknown_locked_outputs:
                is_unlocked = get_is_outputs_unlocked(unknown_locked_outputs)
                assert len(is_unlocked) == len(unknown_locked_outputs)
                for i, o in enumerate(unknown_locked_outputs):
                    is_unlocked_cache[o] = is_unlocked[i]
                num_picks_unknown_locked = 0

            # Yield the buffered picks
            for buffered_pick in buffered_picks:
                # If pick is locked, skip to next buffered pick
                if not is_unlocked_cache[buffered_pick]:
                    continue

                yield buffered_pick
                num_total_picked += 1

            # Clear buffer
            buffered_picks.clear()

def main():
    # Handle CLI arguments
    arg_parser = argparse.ArgumentParser(prog='Decoy Selection Python Reference',
                    description='We provide an easy-to-read non-fingerprinting reference for ' \
                        'Monero decoy selection',
                    epilog='Remember: Don\'t be Unique!')
    arg_parser.add_argument('-n', '--num-picks', default=1000000, type=int)
    arg_parser.add_argument('-o', '--output-file-prefix', default='decoy_selections')
    arg_parser.add_argument('-d', '--daemon-host', default='127.0.0.1')
    arg_parser.add_argument('-p', '--daemon-port', default=18081, type=int)
    arg_parser.add_argument('--allow-output-overwrite', action='store_true')
    arg_parser.add_argument('--allow-chain-update', action='store_true')
    args = arg_parser.parse_args()

    # Create connection to monerod
    daemon = framework.daemon.Daemon(host=args.daemon_host, port=args.daemon_port)

    # Fetch the top block hash at the beginning of the picking. We will do this again at the end to
    # assert that the unlocked status of the set of all outputs didn't change. This is a practial
    # detail that makes conformance testing more consistent
    early_top_block_hash = daemon.get_info().top_block_hash

    # Construct output file name and check that it doesn't already exist
    output_file_name = "{}_{}_{}.dat".format(args.output_file_prefix, early_top_block_hash,
        args.num_picks)
    if os.path.isfile(output_file_name) and not args.allow_output_overwrite:
        print("File '{}' already exists".format(output_file_name), file=sys.stderr)
        exit(1)

    # Fetch the CROD
    print("Fetching the CROD as of block {} from daemon '{}:{}'...".format(
        early_top_block_hash, args.daemon_host, args.daemon_port))
    res = daemon.get_output_distribution(amounts=[0], cumulative=False)
    rct_dist_info = res['distributions'][0]
    crod = rct_dist_info['distribution']
    assert rct_dist_info['base'] == 0
    print("The start height of the CROD is {}, and the top height is {}.".format(
        rct_dist_info['start_height'], rct_dist_info['start_height'] + len(crod) - 1))

    # Accumulate the CROD since it is fetched uncumulative for compactness over the wire
    for i in range(len(crod) - 1):
        crod[i + 1] += crod[i]

    # Define our unlockedness fetcher: this functor simply takes a list of RCT global indexes
    # and returns a list of true/false if is unlocked for each index using RPC endpoint /get_outs
    def get_is_outputs_unlocked(rct_output_indices):
        res = daemon.get_outs([{'index': o, 'amount': 0} for o in rct_output_indices])
        assert len(res.outs) == len(rct_output_indices)
        return [o.unlocked for o in res.outs]

    # Main gamma picking / output loop
    print("Performing {} picks and writing output to '{}'...".format(args.num_picks, output_file_name))
    with open(output_file_name, 'w') as outf:
        for i, pick in enumerate(gamma_pick_n_unlocked(args.num_picks, crod, get_is_outputs_unlocked)):
            # Print progress
            print_period = args.num_picks // 1000 if args.num_picks >= 1000 else 1
            if (i+1) % print_period == 0:
                progress = (i+1) / args.num_picks * 100
                print("Progress: {:.1f}%".format(progress), end='\r')

            # Write pick to file
            print(pick, file=outf)

    print()

    # Fetch the top block hash at the end of the picking and check that it matches the top block
    # hash at the beginning of the picking. If it doesn't, then exit with a failure message and
    # delete the data file (if enforcing). There is a tiny chance that we start with block A,
    # reorg to B, then reorg back to A, but this unlikely scenario is probably not worth handling.
    later_top_block_hash = daemon.get_info().top_block_hash
    if later_top_block_hash != early_top_block_hash:
        print("The top block hash changed from {} to {}! This will harm statistical analysis!".
            format(early_top_block_hash, later_top_block_hash), file=sys.stderr)

        if not args.allow_chain_update:
            os.remove(output_file_name)
            print("This script enforces that we start and finish with the same top block hash "
                "so we can get more consistent results. If you want to ensure that your node "
                "doesn't update its blockchain state, you can start it offline with the CLI flag "
                "--offline. Alternatively, you can run this script with the CLI flag "
                "--allow-chain-update.")
            exit(1)

if __name__ == '__main__':
    main()
