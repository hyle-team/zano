import sys
import os
import smtplib
from email.message import EmailMessage

def getenv(e):
    t = os.getenv(e)
    if t == None:
        print("Error: environment variable " + e + " was not set")
        exit(1)
    return t

zs_from = getenv("ZANO_SMTP_FROM")
zs_addr = getenv("ZANO_SMTP_ADDR")
zs_port = getenv("ZANO_SMTP_PORT")
zs_user = getenv("ZANO_SMTP_USER")
zs_pass = getenv("ZANO_SMTP_PASS")

if len(sys.argv) != 4:
    print("Usage: " + sys.argv[0] + " <subject> <recipient(s)> <body>")
    exit(1)

msg = EmailMessage()
msg['Subject'] = sys.argv[1]
msg['From'] = zs_from
msg['To'] = sys.argv[2]
msg.add_header('Content-Type','text/html')
msg.set_payload(sys.argv[3])

s = smtplib.SMTP(zs_addr, zs_port)
s.starttls()
s.login(zs_user, zs_pass)
s.send_message(msg)
s.quit()

print("e-mail sent.")
