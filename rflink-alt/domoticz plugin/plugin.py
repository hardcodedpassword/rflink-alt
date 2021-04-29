# Basic Python Plugin Example For RFLink-alt and Domoticz
# This implements two switches to control by RFLink-alt
# 
# From: GizMoCuz, Modified for RFLink by HardcodedPassword
#
"""
<plugin key="RFLink-alt" name="RFLink-alt" author="mbg" version="1.0.0" wikilink="http://www.domoticz.com/wiki/plugins/plugin.html" externallink="https://www.google.com/">
    <description>
        <h2>RFLink-alt</h2><br/>
        Overview...
        <h3>Features</h3>
        <ul style="list-style-type:square">
            <li>Feature one...</li>
            <li>Feature two...</li>
        </ul>
        <h3>Devices</h3>
        <ul style="list-style-type:square">
            <li>Device Type - What it does...</li>
        </ul>
        <h3>Configuration</h3>
        Configuration options...
    </description>
    <params>
        <param field="SerialPort" label="Serial Port" width="150px" required="true" default="/dev/ttyS0"/>
        <param field="Mode6" label="Debug" width="100px">
            <options>
                <option label="True" value="Debug"/>
                <option label="False" value="Normal"  default="true" />
                <option label="Logging" value="File"/>
            </options>
        </param>  
    </params>
</plugin>
"""
import Domoticz
import array as arr

connection = None
class BasePlugin:
    enabled = False
    
    def __init__(self):
        self.RxBuf = ""
        return

    def onStart(self):
        global connection
        if Parameters["Mode6"] != "Normal":
            Domoticz.Debugging(1)
        if Parameters["Mode6"] == "Debug":
            f = open(Parameters["HomeFolder"]+"plugin.log","w")
            f.write("Plugin started.")
            f.close()

        LogMessage("onStart called")
        
        if (len(Devices) == 0):
            Domoticz.Device(Name="Serre",     Unit=1,  Type=244, Subtype=73, Switchtype=0, Used=1, Image=0).Create()
            Domoticz.Device(Name="Afzuigkap", Unit=2,  Type=244, Subtype=73, Switchtype=0, Used=1, Image=0).Create()
         
        Devices[1].Update(0, "Off")
        Devices[2].Update(0, "Off")
        DumpConfigToLog()
        
        connection = Domoticz.Connection(Name="RFLink-alt", Transport="Serial", Protocol="None", Address=Parameters["SerialPort"], Baud=57600)
        connection.Connect()
        LogMessage("Connected")
                
    def onStop(self):
        LogMessage("onStop called")

    def onConnect(self, Connection, Status, Description):
        global connection
        LogMessage("onConnect called")
        if (Status == 0):
            LogMessage("Connected successfully to: "+Parameters["SerialPort"])
            connection = Connection
        else:
            LogMessage("Failed to connect ("+str(Status)+") to: "+Parameters["SerialPort"]+" with error: "+Description)
        self.RxBuf =""
        return True        

    def onMessage(self, Connection, Data):
        # Data is a byte array
        self.RxBuf = self.RxBuf + Data.decode("utf-8")
        
        if Data[-1] == 0x0A:
            LogMessage(self.RxBuf)
            self.RxBuf = "" 
                
    def onCommand(self, Unit, Command, Level, Hue):
        LogMessage("onCommand called for Unit " + str(Unit) + ": Parameter '" + str(Command) + "', Level: " + str(Level))
        if (Unit == 1):
            # Serre - hamuled
            connection.Send("t:10,40,THE PULSE SEQUENCE GOES HERE");
            if (Command == "On" ):
                Devices[1].Update(1, "On")
            else:
                Devices[1].Update(0, "Off")
                
        if (Unit == 2):
            # Kitchen bosch hood lights
            connection.Send("t:10,40,THE PULSE SEQUENCE GOES HERE")
            if (Command == "On" ):
                Devices[2].Update(1, "On")
            else:
                Devices[2].Update(0, "Off")


    def onNotification(self, Name, Subject, Text, Status, Priority, Sound, ImageFile):
        LogMessage("Notification: " + Name + "," + Subject + "," + Text + "," + Status + "," + str(Priority) + "," + Sound + "," + ImageFile)

    def onDisconnect(self, Connection):
        LogMessage("onDisconnect called")

    def onHeartbeat(self):
        pass


global _plugin
_plugin = BasePlugin()

def onStart():
    global _plugin
    _plugin.onStart()

def onStop():
    global _plugin
    _plugin.onStop()

def onConnect(Connection, Status, Description):
    global _plugin
    _plugin.onConnect(Connection, Status, Description)

def onMessage(Connection, Data):
    global _plugin
    _plugin.onMessage(Connection, Data)

def onCommand(Unit, Command, Level, Hue):
    global _plugin
    _plugin.onCommand(Unit, Command, Level, Hue)

def onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile):
    global _plugin
    _plugin.onNotification(Name, Subject, Text, Status, Priority, Sound, ImageFile)

def onDisconnect(Connection):
    global _plugin
    _plugin.onDisconnect(Connection)

def onHeartbeat():
    global _plugin
    _plugin.onHeartbeat()

    # Generic helper functions
    

def LogMessage(Message):
    if Parameters["Mode6"] == "Debug":
        f = open(Parameters["HomeFolder"]+"plugin.log","a")
        f.write(Message+"\r\n")
        f.close()
    Domoticz.Debug(Message)
    
def DumpConfigToLog():
    for x in Parameters:
        if Parameters[x] != "":
            LogMessage( "'" + x + "':'" + str(Parameters[x]) + "'")
    LogMessage("Device count: " + str(len(Devices)))
    for x in Devices:
        LogMessage("Device:           " + str(x) + " - " + str(Devices[x]))
        LogMessage("Device ID:       '" + str(Devices[x].ID) + "'")
        LogMessage("Device Name:     '" + Devices[x].Name + "'")
        LogMessage("Device nValue:    " + str(Devices[x].nValue))
        LogMessage("Device sValue:   '" + Devices[x].sValue + "'")
        LogMessage("Device LastLevel: " + str(Devices[x].LastLevel))
    return
