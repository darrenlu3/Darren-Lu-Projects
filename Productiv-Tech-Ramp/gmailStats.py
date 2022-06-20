"""
Built off Python quickstart code from Google's intro to GMail API
https://developers.google.com/gmail/api/quickstart/python
"""

from __future__ import print_function

import os.path
import json
import boto3
import logging

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

# If modifying these scopes, delete the file token.json.
SCOPES = ['https://www.googleapis.com/auth/gmail.readonly']
USER_EMAIL = "darrenlu3@g.ucla.edu"

fromGmail = []
fromNonGmail = []
toGmail = []
toNonGmail=[]

def getGmailStats():
    """Shows basic usage of the Gmail API.
    Lists the user's Gmail labels.
    """
    creds = None
    # The file token.json stores the user's access and refresh tokens, and is
    # created automatically when the authorization flow completes for the first
    # time.
    if os.path.exists('token.json'):
        creds = Credentials.from_authorized_user_file('token.json', SCOPES)
    # If there are no (valid) credentials available, let the user log in.
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(
                'credentials.json', SCOPES)
            creds = flow.run_local_server(port=0)
        # Save the credentials for the next run
        with open('token.json', 'w') as token:
            token.write(creds.to_json())

    # Given a value field in a message object, extract the email and domain associated with the value
    def extractEmail(email):
        params = email.split()
        for param in params:
            if '@' in param:
                email = param
                break
        email = email.replace('>', '')
        email = email.replace('<', '')
        domain = email[email.find('@'):]
        return email, domain

    try:
        # Call the Gmail API
        service = build('gmail', 'v1', credentials=creds)
        results = service.users().messages().list(userId='me').execute()
        messages = results.get('messages', [])

        if not messages:
            logging.info('No messages found.')
            return
        for i, message in enumerate(messages):
            # Extract object data from message object
            messageId = message['id']
            messageObj = service.users().messages().get(userId='me', id=messageId).execute()
            timestamp = messageObj['internalDate']
            #timestamp = datetime.datetime.fromtimestamp(float(timestamp) / 1000.0)
            payload = messageObj['payload']
            headers = payload['headers']

            sender = None
            senderDomain = None
            receiver = None
            receiverDomain = None
            for header in headers:
                if header['name'] == 'From':
                    sender = header['value']
                if header['name'] == 'To':
                    receiver = header['value']
                else:
                    continue
                if sender is not None and receiver is not None:
                    break

            # Extract email addresses from header values
            if sender is None:
                logging.error("No sender in header field for this message!")
            else:
                sender, senderDomain = extractEmail(sender)
            if receiver is None:
                logging.error("No receiver in header field for this message!")
            else:
                receiver, receiverDomain = extractEmail(receiver)

            if sender == USER_EMAIL:
                # Sent from me to a gmail user
                if receiverDomain == "@gmail.com":
                    toGmail.append({"messageId": messageId, "receiver": receiver, "timestamp": timestamp})
                else:
                    toNonGmail.append({"messageId": messageId, "receiver": receiver, "timestamp": timestamp})
            elif receiver == USER_EMAIL:
                if senderDomain == "@gmail.com":
                    fromGmail.append({"messageId": messageId, "sender": sender, "timestamp": timestamp})
                else:
                    fromNonGmail.append({"messageId": messageId, "sender": sender, "timestamp": timestamp})

    except HttpError as error:
        # TODO(developer) - Handle errors from gmail API.
        logging.error(f'An error occurred: {error}')

def uploadToDynamoDb():
    """Takes extracted gmail metrics and uplaods to Dynamo DB"""
    db = boto3.resource('dynamodb')
    toGmailTable = db.Table('Gmail-Stats-ToGmail')
    toNonGmailTable = db.Table('Gmail-Stats-ToNonGmail')
    fromGmailTable = db.Table('Gmail-Stats-FromGmail')
    fromNonGmailTable = db.Table('Gmail-Stats-FromGmail')

    def uploadToTable(table, list):
        items = list
        for item in items:
            #print(item)
            table.put_item(Item=item)

    uploadToTable(toGmailTable, toGmail)
    uploadToTable(toNonGmailTable, toNonGmail)
    uploadToTable(fromGmailTable, fromGmail)
    uploadToTable(fromNonGmailTable, fromNonGmail)



if __name__ == '__main__':
    getGmailStats()
    uploadToDynamoDb()