'''
Created on 29 Mar 2017

@author: rubenmennes
'''

from queue import Queue, Empty
import threading

from communicator.python.Communicator import Message, CommManager
import communicator.python.interf_pb2 as interf


#===============================================================================
# LayerCommunicator
# This Class is responsible for the communication setup and setting up the queues
#===============================================================================
class LayerCommunicator(object):
    '''
    classdocs
    '''


    #===========================================================================
    # __init__
    # Initialize the LayerCommunicator
    # module must be a valid interf.MODULE
    # queueSize is the maximum queue size of a queue in the LayerCommunicator (0 if unbouded)
    #===========================================================================
    def __init__(self, module, other_modules=[], queueSize=0):
        '''
        Constructor
        '''
        self.sendingQueue = Queue(queueSize)
        self.highQueue = Queue(queueSize)
        self.lowQueue = Queue(queueSize)

        self.threads = {}

        self.source_module = module

        print("Start threads")
        for i in other_modules:
            print("\tStart thread " + str(i))
            if module == i:
                continue
            self.threads[i] = (CommManager(module, i), threading.Thread(target=self._receive, args=(i, ), daemon=True))
            self.threads[i][1].start()

        self.sendingThread = threading.Thread(target=self._sending, daemon=True)
        self.sendingThread.start()

    def getModule(self):
        return self.source_module

    #===========================================================================
    # get_low_queue
    # Get the Thread-Safe queue for the low_priority messages
    #===========================================================================
    def get_low_queue(self):
        return self.lowQueue

    #===========================================================================
    # get_high_queue
    # Get the Thread-Safe queue for the high_priority messages
    #===========================================================================
    def get_high_queue(self):
        return self.highQueue

    #===========================================================================
    # send
    # Add a new message in the queue
    #===========================================================================
    def send(self, message: Message):
        self.sendingQueue.put(message)

    #===========================================================================
    # filter
    # Filter the high priority messages and low priority messages
    # High priority = True, Low priority = False
    #===========================================================================
    def filter(self, message: Message):
        tmp = message.message.WhichOneof("payload")

        if tmp is None:
            return None
        elif tmp in ['get', 'getr', 'set', 'setr', 'stats']:
            return True
        else:
            return False

    #===========================================================================
    # _receive
    # The receive thread
    #===========================================================================
    def _receive(self, module):
        comm = self.threads[module][0]
        while(True):
            try:
                m = comm.receive(1000)  # WAIT FOR NEW MESSAGE FORM ZMQ
                if m is None:
                    continue
                tmp = self.filter(m)
                if tmp is None:
                    continue
                elif tmp:
                    self.highQueue.put(m)
                else:
                    self.lowQueue.put(m)
            except Exception as e:
                print(e)

    #===========================================================================
    # _sending
    # The sending thread
    #===========================================================================
    def _sending(self):
        while(True):
            try:
                m = self.sendingQueue.get(timeout=1)
                self.threads[m.destination][0].send(m)
            except Empty:
                pass
