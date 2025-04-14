import paho.mqtt.client as mqtt
import json

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe("devices/lora/807B8590200005EC/#")

def on_message(client, userdata, msg):
    print(f"Topic: {msg.topic}, Message: {msg.payload}")
    
    try:
        data = json.loads(msg.payload)
        temperature = data['data']['temperature']
        humidity = data['data']['humidity']
        
        if temperature < 10 or temperature > 30:
            print(f"yoooooooooooo! too hot!: {temperature}")
        
        if humidity > 50:
            print(f"yoooooooooooooo! too humid!: {humidity}")
        
    except Exception as e:
        print(f"Error parsing message: {e}")

client_name = 'alert_system'
client = mqtt.Client(client_name)

client.on_connect = on_connect
client.on_message = on_message

client.connect('10.96.4.11', 1883, 60)

client.loop_forever()
