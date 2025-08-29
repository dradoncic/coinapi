import asyncio
import json
import ssl
import websockets
import jwt
import time
import uuid

WS_URL = "wss://ws-feed.exchange.coinbase.com"
API_KEY = ""
SIGNING_KEY = ""

def generate_jwt():
    current_time = int(time.time())
    payload = {
        "iss": "cdp",
        "nbf": current_time,
        "exp": current_time + 120,  # valid for 120 seconds
        "sub": API_KEY,
    }
    headers = {
        "kid": API_KEY,
        "nonce": uuid.uuid4().hex
    }
    token = jwt.encode(payload, SIGNING_KEY, algorithm="ES256", headers=headers)
    return token

async def coinbase_websocket():
    # Create SSL context that ignores certificate verification
    ssl_context = ssl.create_default_context()
    ssl_context.check_hostname = False
    ssl_context.verify_mode = ssl.CERT_NONE
    
    try:
        async with websockets.connect(WS_URL, ssl=ssl_context) as websocket:
            print("Connected to Coinbase WebSocket")
            
            # Subscribe to BTC-USD ticker
            subscribe_message = {
                "type": "subscribe",
                "product_ids": ["BTC-USD"],
                "channels": ["ticker"],
                "jwt": generate_jwt()
            }
            
            await websocket.send(json.dumps(subscribe_message))
            print("Subscribed to BTC-USD ticker")
            
            # Listen for messages
            async for message in websocket:
                try:
                    data = json.loads(message)
                    print(f"Received: {data}")
                except json.JSONDecodeError as e:
                    print(f"JSON decode error: {e}")
                    print(f"Raw message: {message}")
                    
    except Exception as e:
        print(f"Connection error: {e}")

if __name__ == "__main__":
    asyncio.run(coinbase_websocket())