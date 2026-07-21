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

args = sys.argv[1:]
body_file = None
if "--body-file" in args:
    i = args.index("--body-file")
    body_file = args[i + 1]
    del args[i:i + 2]

if len(args) != 3:
    print("Usage: " + sys.argv[0] + " <subject> <recipient(s)> <body> [--body-file FILE]")
    exit(1)

subject, recipients, body = args
if body_file:
    with open(body_file, "r", encoding="utf-8") as f:
        body += f.read()

msg = EmailMessage()
msg['Subject'] = subject
msg['From'] = zs_from
msg['To'] = recipients
msg.add_header('Content-Type','text/html')
msg.set_payload(body, 'utf-8')   # charset -> non-ASCII bodies serialize/send correctly

s = smtplib.SMTP(zs_addr, zs_port)
s.starttls()
s.login(zs_user, zs_pass)
s.send_message(msg)
s.quit()

print("e-mail sent.")
