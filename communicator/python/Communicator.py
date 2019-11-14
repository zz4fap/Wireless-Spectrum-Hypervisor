# Communicator for SCATTER process
#
# Author: Miguel Camelo
#
#   This file implements the communication modules for AI, MAC and Phy.

from pathlib import Path
import zmq

import communicator.python.interf_pb2 as interf

IPCDIR = "/tmp/IPC_"

def touch(fname):
    Path(fname).touch()

def createUniqueFile(ownModule, otherModule, push):
    if push:
        return IPCDIR + str(ownModule) + "_" + str(otherModule)
    else:
        return IPCDIR + str(otherModule) + "_" + str(ownModule) 


#===============================================================================
# Message
# The message class represent a complete message
# You can access all variables
#===============================================================================
class Message(object):

    def __init__(self, source=0, dest=0, message=None):
        self.source = source
        self.destination = dest
        self.message = message

    def __str__(self):
        return "Direction: " + interf.MODULE.Name(self.source) + " => " + interf.MODULE.Name(self.destination) + \
                "\n" + str(self.message)


#===============================================================================
# CommManager
# Commmunicator Manager will be responsable to setup a correct ZMQ publisher/subscriber
#===============================================================================
class CommManager():

    #===========================================================================
    # __init__
    # Initialize owner needs to be a valid interf.MODULE
    #===========================================================================
    def __init__(self, owner, other_module):

        if owner in interf.MODULE.values() and other_module in interf.MODULE.values():
            self.owner = owner
            self.other_module = other_module
            # Create context
            self.context = zmq.Context(2)
            
            #create IPC file
            self.pushfile = createUniqueFile(owner, other_module, True)
            self.pullfile = createUniqueFile(owner, other_module, False)
            touch(self.pushfile)
            touch(self.pullfile)
            ipcPush = "ipc://" + self.pushfile
            ipcPull = "ipc://" + self.pullfile

            self.socketPush = self.context.socket(zmq.PUSH)
            self.socketPull = self.context.socket(zmq.PULL)
            
            self.socketPush.bind(ipcPush)
            self.socketPull.connect(ipcPull)
            
            self.poller = zmq.Poller()
            self.poller.register(self.socketPull, zmq.POLLIN)
            
            print("Communicator manager in " + interf.MODULE.Name(owner) + " started to " + interf.MODULE.Name(other_module))

        else:
            print ("Manager could not start, unknown SCATTER module")
            return None

    #===========================================================================
    # close
    # Close the socket
    #===========================================================================
    def close(self):
        self.socketPull.close()
        self.socketPush.close()

    #===========================================================================
    # send
    # Send a message (all ascii encoded)
    #===========================================================================
    def send(self, message : Message):

        return self.socketPush.send_multipart([message.message.SerializeToString()])


    #===========================================================================
    # receive
    # Receive a new Message (blocked until new message in ZMQ Queue
    #===========================================================================
    def receive(self, timeout):
        socks = dict(self.poller.poll(timeout))
        if self.socketPull in socks and socks[self.socketPull] == zmq.POLLIN:
            
            receive_envelop = self.socketPull.recv_multipart()

            return Message(
                source=self.other_module,
                dest=self.owner,
                message=self._deserializedData(receive_envelop[0])
            )
        else:
            return None

    #===========================================================================
    # _deserializedData
    # Deserialize the correct protobuf data
    #===========================================================================
    def _deserializedData(self, data):
        data_recoverd = interf.Internal()
        data_recoverd.ParseFromString(data)
        return data_recoverd
