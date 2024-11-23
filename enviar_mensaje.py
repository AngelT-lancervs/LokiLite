import sys
from twilio.rest import Client
from dotenv import load_dotenv
import os

load_dotenv()

if len(sys.argv) < 2:
    print("Debe proporcionar el nombre del servicio como argumento.")
    sys.exit(1)

service_name = sys.argv[1]

account_sid = os.getenv("TWILIO_ACCOUNT_SID")
auth_token = os.getenv("TWILIO_AUTH_TOKEN")
from_whatsapp = 'whatsapp:+14155238886' 

to_whatsapp = 'whatsapp:+593980740851'

message_body = f"¡ALERTA! El servicio {service_name} ha superado el Treshold (20) de logs."

client = Client(account_sid, auth_token)

try:
    message = client.messages.create(
        body=message_body,
        from_=from_whatsapp,
        to=to_whatsapp
    )
    print(f"Mensaje enviado exitosamente a WhatsApp. SID: {message.sid}")
except Exception as e:
    print(f"Error al enviar mensaje: {e}")
