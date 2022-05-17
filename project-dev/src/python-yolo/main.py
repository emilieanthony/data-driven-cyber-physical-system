import sys
import struct

# Default messages from libcluon providing Envelope and TimeStamp
import cluonDataStructures_pb2
# OpenDLV Standard Message Set
import opendlv_standard_message_set_v0_9_9_pb2
# Messages from individual message set
import example_pb2
import csv
import numpy as np
from array import array
import cv2

def extractImage(dataType, payload):
    if dataType == 1055:
        messageFromPayload = opendlv_standard_message_set_v0_9_9_pb2.opendlv_proxy_ImageReading()
        messageFromPayload.ParseFromString(payload)
        print("Payload: %s" % (str(messageFromPayload)))
        while True:
            fileSizeBytes = messageFromPayload.data
            fileSize = 0
            for i in range(4):
                fileSize += array('B',fileSizeBytes[i + 2])[0] * 256 ** i
            bmpData = fileSizeBytes + ffmpeg.stdout.read(fileSize - 6)
            image = cv2.imdecode(np.fromstring(bmpData, dtype = np.uint8), 1)
            cv2.imshow("im",image) 
            cv2.waitKey(25)
    
            



        
        
       
        

        
        
def extractGroundSteerRequest(dataType, payload):
    if dataType == 1090:
        messageFromPayload = opendlv_standard_message_set_v0_9_9_pb2.opendlv_proxy_GroundSteeringRequest()
        messageFromPayload.ParseFromString(payload)
        print("groundSteering: %s" % (str(messageFromPayload.groundSteering)))
    
################################################################################
# Print an Envelope's meta information.
def printEnvelope(e):
    print("Envelope ID/senderStamp = %s/%s" % (str(e.dataType), str(e.senderStamp)))
    print(" - sent                 = %s.%s" % (str(e.sent.seconds), str(e.sent.microseconds)))
    print(" - received             = %s.%s" % (str(e.received.seconds), str(e.received.microseconds)))
    print(" - sample time          = %s.%s" % (str(e.sampleTimeStamp.seconds), str(e.sampleTimeStamp.microseconds)))
    extractImage(e.dataType, e.serializedData)
    print()

    
################################################################################
# Main entry point.
if len(sys.argv) != 2:
    print("Display Envelopes captured from OpenDLV.")
    print("  Usage: %s example.rec" % (str(sys.argv[0])))
    sys.exit()

# Read Envelopes from .rec file.
with open(sys.argv[1], "rb") as f:
    buf = b""
    bytesRead = 0
    expectedBytes = 0
    LENGTH_ENVELOPE_HEADER = 5
    consumedEnvelopeHeader = False
    byte = f.read(1)
    while byte != "":
        buf =  b"".join([buf, byte])
        bytesRead = bytesRead + 1

        if consumedEnvelopeHeader:
            if len(buf) >= expectedBytes:
                envelope = cluonDataStructures_pb2.cluon_data_Envelope()
                envelope.ParseFromString(buf)
                printEnvelope(envelope)
                # Start over and read next container.
                consumedEnvelopeHeader = False
                buf = buf[expectedBytes:]
                expectedBytes = 0

        if not consumedEnvelopeHeader:
            if len(buf) >= LENGTH_ENVELOPE_HEADER:
                consumedEnvelopeHeader = True
                byte0 = buf[0]
                byte1 = buf[1]

                # Check for Envelope header.
                if byte0 == 0x0D and byte1 == 0xA4:
                    v = struct.unpack('<L', buf[1:5]) # Read uint32_t and convert to little endian.
                    expectedBytes = v[0] >> 8 # The second byte belongs to the header of an Envelope.
                    buf = buf[5:] # Remove header.
                else:
                    print("Failed to consume header from Envelope.")

        # Read next byte.
        byte = f.read(1)

        # End processing at file's end.
        if not byte:
            break


f.close()