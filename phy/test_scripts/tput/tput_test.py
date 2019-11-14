import select
import socket
import time
import _thread
import threading
import getopt
import queue
import random
from time import sleep
import signal, os

import sys
sys.path.append('../../../')
sys.path.append('../../../communicator/python/')

from communicator.python.Communicator import Message
from communicator.python.LayerCommunicator import LayerCommunicator
import communicator.python.interf_pb2 as interf

PRB_VECTOR = [6, 15, 25, 50]

NOF_SLOTS_VECTOR = [20]

tx_exit_flag = False
def handler(signum, frame):
    global tx_exit_flag
    tx_exit_flag = True

def getExitFlag():
    global tx_exit_flag
    return tx_exit_flag

def kill_phy():
    os.system("~/radio_api/stop.sh")
    os.system("~/radio_api/kill_stack.py")
    os.system("killall -9 trx")
    os.system("killall -9 measure_throughput_fd_auto")

def start_phy(bw, center_freq, comp_bw):
    cmd1 = "sudo ../../../build/phy/srslte/examples/trx -f " + str(center_freq) + " -B " + str(comp_bw) + " -p " + str(bw) + " &"
    os.system(cmd1)

def start_phy_with_filter(bw, center_freq, comp_bw):
    cmd1 = "sudo ../../../build/phy/srslte/examples/trx -f " + str(center_freq) + " -B " + str(comp_bw) + " -p " + str(bw) + " -QQ &"
    os.system(cmd1)

def start_tput_measurement(nof_prb, nof_slots, mcs, txgain, rxgain):
    cmd1 = "sudo ../../../build/phy/srslte/examples/measure_throughput_fd_auto -b " + str(txgain) + " -g " + str(rxgain) + " -n " + str(nof_slots) + " -m " + str(mcs) + " -p " + str(nof_prb)
    os.system(cmd1)

def start_scenario():
    cmd1 = "colosseumcli rf start 8981 -c"
    os.system(cmd1)

def save_files(nof_prb):
    cmd1 = "mkdir dir_tput_prb_" + str(nof_prb)
    os.system(cmd1)
    cmd2 = "mv tput_prb_* dir_tput_prb_" + str(nof_prb)
    os.system(cmd2)
    cmd3 = "tar -cvzf" + " dir_tput_prb_" + str(nof_prb) + ".tar.gz"  + " dir_tput_prb_" + str(nof_prb) + "/"
    os.system(cmd3)

def save_files_filter(nof_prb):
    cmd1 = "mkdir dir_tput_filter_128_prb_" + str(nof_prb)
    os.system(cmd1)
    cmd2 = "mv tput_prb_* dir_tput_filter_128_prb_" + str(nof_prb)
    os.system(cmd2)
    cmd3 = "tar -cvzf" + " dir_tput_filter_128_prb_" + str(nof_prb) + ".tar.gz"  + " dir_tput_filter_128_prb_" + str(nof_prb) + "/"
    os.system(cmd3)

def get_bw_from_prb(nof_prb):
    bw = 0
    if(nof_prb == 6):
        bw = 1400000
    elif(nof_prb == 15):
        bw = 3000000
    elif(nof_prb == 25):
        bw = 5000000
    elif(nof_prb == 50):
        bw = 10000000
    else:
        printf("Invalid number of PRB")
        exit(-1)
    return bw

def inputOptions(argv):
    nof_prb = 25 # By dafult we set the number of resource blocks to 25, i.e., 5 MHz bandwidth.
    mcs = 0 # By default MCS is set to 0, the most robust MCS.
    txgain = 20 # By default TX gain is set to 20.
    rxgain = 20 # By default RX gain is set to 20.
    tx_slots = 1
    tx_channel = 0
    rx_channel = 0
    run_scenario = False
    use_filter = False
    center_freq = 2000000000
    comp_bw = 20000000

    try:
        opts, args = getopt.getopt(argv,"h",["help","bw=","mcs=","txgain=","rxgain=","txslots=","txchannel=","rxchannel=","runscenario","usefilter","centerfreq=","compbw="])
    except getopt.GetoptError:
        help()
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("--help"):
            sys.exit()
        elif opt in ("--bw"):
            nof_prb = int(arg)
        elif opt in ("--mcs"):
            mcs = int(arg)
        elif opt in ("--txgain"):
            txgain = int(arg)
        elif opt in ("--rxgain"):
            rxgain = int(arg)
        elif opt in ("--txslots"):
            tx_slots = int(arg)
        elif opt in ("--txchannel"):
            tx_channel = int(arg)
        elif opt in ("--rxchannel"):
            rx_channel = int(arg)
        elif opt in ("--runscenario"):
            run_scenario = True
        elif opt in ("--usefilter"):
            use_filter = True
        elif opt in ("--centerfreq"):
            center_freq = int(arg)
        elif opt in ("--compbw"):
            comp_bw = int(arg)

    return nof_prb, mcs, txgain, rxgain, tx_slots, tx_channel, rx_channel, run_scenario, use_filter, center_freq, comp_bw

if __name__ == '__main__':

    # Set the signal handler.
    signal.signal(signal.SIGINT, handler)

    nof_prb, mcs, txgain, rxgain, tx_slots, tx_channel, rx_channel, run_scenario, use_filter, center_freq, comp_bw = inputOptions(sys.argv[1:])

    if(run_scenario == True):
        start_scenario()

    time.sleep(10)

    for prb_idx in range(0,len(PRB_VECTOR)):

        nof_prb = PRB_VECTOR[prb_idx]
        bw = get_bw_from_prb(nof_prb)

        for mcs_idx in range(0,29,2):

            for nof_slots_idx in range(0,len(NOF_SLOTS_VECTOR)):

                # Make sure PHY is not running.
                kill_phy()

                time.sleep(5)

                # Start PHY.
                if(use_filter == True):
                    start_phy_with_filter(bw, center_freq, comp_bw)
                else:
                    start_phy(bw, center_freq, comp_bw)

                time.sleep(15)

                nof_slots = NOF_SLOTS_VECTOR[nof_slots_idx]

                start_tput_measurement(nof_prb, nof_slots, mcs_idx, txgain, rxgain)

                if(getExitFlag() == True):
                    break

            if(getExitFlag() == True):
                break

        if(use_filter == True):
            save_files_filter(nof_prb)
        else:
            save_files(nof_prb)

        if(getExitFlag() == True):
            break
