import time,sys
from twython import TwythonStreamer

# Search terms
TERMS = '#MarvellIoT'


# Twitter application authentication
# replace with your credentials
APP_KEY = ''
APP_SECRET = ''
OAUTH_TOKEN = ''
OAUTH_TOKEN_SECRET = ''

# AWS settings
REST_API_ENDPOINT = 'https://a3jpxg8exwk4qx.iot.us-west-2.amazonaws.com/things/marvell1/shadow'

# Setup callbacks from Twython Streamer
class BlinkyStreamer(TwythonStreamer):
        def on_success(self, data):
                if 'text' in data:
                        print data['text'].encode('utf-8')
			

# Create streamer
try:
        stream = BlinkyStreamer(APP_KEY, APP_SECRET, OAUTH_TOKEN, OAUTH_TOKEN_SECRET)
        stream.statuses.filter(track=TERMS)
except KeyboardInterrupt:
    sys.exit()
