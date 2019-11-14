'''
Created on 30 Mar 2017

@author: rubenmennes
'''

import queue
from time import sleep

from communicator.python.Communicator import Message
from communicator.python.LayerCommunicator import LayerCommunicator
import communicator.python.interf_pb2 as interf


if __name__ == '__main__':
    #LOAD THE LAERY COMMUNICATOR
    module = interf.MODULE_PHY
    lc = LayerCommunicator(module, [interf.MODULE_MAC])

    #SLEEP FOR 4 SECS, TO SETUP THE OTHER MODULES
    sleep(4)

    print("START SENDING")

    #CREATE A PHY_STAT MESSAGE
    stat = interf.Internal()
    stat.transaction_index
    stat.get.attribute = interf.Get.MAC_STATS

    #ADD MESSAGE TO THE SENDING QUEUE
    lc.send(Message(module, interf.MODULE_MAC, stat))

    #GET THE HIGH PRIORITY QUEUE AND WAIT FOR A MESSAGE
    m = lc.get_high_queue().get()
    print("Get message " + str(m))
    #print("TRX Flag: "+str(m.message.trx_flag)+"\nseq_number: "+str(m.message.seq_number)+"\ncenter_freq: "+str(m.message.center_freq)+"\nbw_index: "+str(m.message.bw_index)+"\nch: "+str(m.message.ch)+"\nslot: "+str(m.message.slot)+"\nmcs: "+str(m.message.mcs)+"\ngain: "+str(m.message.gain)+"\nlength: "+str(m.message.length))

    try:
        #TRY TO GET THE NEXT MESSAGE WITHOUT WAITING
        m = lc.get_high_queue().get_nowait()
        print("Get message " + str(m))
    except queue.Empty:
        print("No message yet")

    sleep(10)
    print("STOP")
