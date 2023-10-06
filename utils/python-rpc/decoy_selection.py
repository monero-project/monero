#!/usr/bin/python3

''' See step-by-step accompaniment in docs/DECOY_SELECTION.md '''

import argparse
import bisect
try:
    import numpy as np
except:
    print('numpy must be installed!')
    exit(1)
import requests
import sys

import framework.daemon

rng = np.random.Generator(np.random.PCG64(seed=None))

# Section: "First, Some Numeric Constants"
GAMMA_SHAPE = 19.28
GAMMA_RATE = 1.61
GAMMA_SCALE = 1 / GAMMA_RATE

CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE = 10
DIFFICULTY_TARGET_V2 = 120
DEFAULT_UNLOCK_TIME = CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE * DIFFICULTY_TARGET_V2
RECENT_SPEND_WINDOW = 15 * DIFFICULTY_TARGET_V2

SECONDS_IN_A_YEAR =  60 * 60 * 24 * 365
BLOCKS_IN_A_YEAR = SECONDS_IN_A_YEAR // DIFFICULTY_TARGET_V2

# Section: "How to calculate `average_output_flow`"
def calculate_average_output_flow(crod):
    # 1
    num_blocks_to_consider_for_flow = min(len(crod), BLOCKS_IN_A_YEAR)

    # 2
    if len(crod) > num_blocks_to_consider_for_flow:
        num_outputs_to_consider_for_flow = crod[-1] - crod[-(num_blocks_to_consider_for_flow + 1)]
    else:
        num_outputs_to_consider_for_flow = crod[-1]
    
    # 3
    average_output_flow = DIFFICULTY_TARGET_V2 * num_blocks_to_consider_for_flow / num_outputs_to_consider_for_flow

    return average_output_flow

# Section: "How to calculate `num_usable_rct_outputs`"
def calculate_num_usable_rct_outputs(crod):
    # 1
    num_usable_crod_blocks = len(crod) - (CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE - 1)

    # 2
    num_usable_rct_outputs = crod[num_usable_crod_blocks - 1]

    return num_usable_rct_outputs

# Section: "The Gamma Pick"
def gamma_pick(crod, average_output_flow, num_usable_rct_outputs):
    while True:
        # 1
        x = rng.gamma(GAMMA_SHAPE, GAMMA_SCALE) # parameterized by scale, not rate!

        # 2
        target_output_age = np.exp(x)

        # 3
        if target_output_age > DEFAULT_UNLOCK_TIME:
            target_post_unlock_output_age = target_output_age - DEFAULT_UNLOCK_TIME
        else:
            target_post_unlock_output_age = np.floor(rng.uniform(0.0, RECENT_SPEND_WINDOW))

        # 4
        target_num_outputs_post_unlock = int(target_post_unlock_output_age / average_output_flow)

        # 5
        if target_num_outputs_post_unlock >= num_usable_rct_outputs:
            continue

        # 6
        psuedo_global_output_index = num_usable_rct_outputs - 1 - target_num_outputs_post_unlock

        # 7
        picked_block_index = bisect.bisect_left(crod, psuedo_global_output_index)

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
        global_output_index_result = int(rng.uniform(block_first_global_output_index, crod[picked_block_index]))

        return global_output_index_result

def main():
    # Handle CLI arguments
    arg_parser = argparse.ArgumentParser(prog='Decoy Selection Python Reference',
                    description='We provide an easy-to-read non-fingerprinting reference for Monero decoy selecton',
                    epilog='Remember: Don\'t be Unique!')
    arg_parser.add_argument('-t', '--to-height', default=0, type=int)
    arg_parser.add_argument('-n', '--num-picks', default=1000000, type=int)
    arg_parser.add_argument('-o', '--output-file', default='python_decoy_selections.txt')
    arg_parser.add_argument('-d', '--daemon-host', default='127.0.0.1')
    arg_parser.add_argument('-p', '--daemon-port', default=18081, type=int)
    args = arg_parser.parse_args()

    # Create connection to monerod
    daemon = framework.daemon.Daemon(host=args.daemon_host, port=args.daemon_port)

    # Fetch the CROD
    print("Fetching the CROD up to height {} from daemon at '{}:{}'...".format(
        '<top>' if args.to_height == 0 else args.to_height, args.daemon_host, args.daemon_port))
    try:
        res = daemon.get_output_distribution(amounts=[0], cumulative=True, to_height=args.to_height)
    except requests.exceptions.ConnectionError:
        print("Error: could not connect to daemon!", file=sys.stderr)
        exit(1)
    rct_dist_info = res['distributions'][0]
    crod = rct_dist_info['distribution']
    assert rct_dist_info['base'] == 0
    print("The start height of the CROD is {}, and the top height is {}.".format(
        rct_dist_info['start_height'], rct_dist_info['start_height'] + len(crod) - 1))

    # Calculate average_output_flow & num_usable_rct_outputs for given CROD
    average_output_flow = calculate_average_output_flow(crod)
    num_usable_rct_outputs = calculate_num_usable_rct_outputs(crod)

    # Do gamma picking and write output
    print("Performing {} picks and writing output to '{}'...".format(args.num_picks, args.output_file))
    print_period = args.num_picks // 1000
    with open(args.output_file, 'w') as outf:
        for i in range(args.num_picks):
            if (i+1) % print_period == 0:
                progress = (i+1) / args.num_picks * 100
                print("Progress: {:.1f}%".format(progress), end='\r')
            pick = gamma_pick(crod, average_output_flow, num_usable_rct_outputs)
            print(pick, file=outf)
        print()

if __name__ == '__main__':
    main()
