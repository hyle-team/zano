const axios = require('axios');
const forge = require('node-forge');

// Your shared JWT secret (must match what the server expects)
const JWT_SECRET = 'hiwejkcddkndspo230XASIijksk123i9x5';


const apiCredentials = {token: JWT_SECRET};

function createJWSToken(payload, secrete_str) {
    const header = { alg: 'HS256', typ: 'JWT' };
    const encodedHeader = Buffer.from(JSON.stringify(header)).toString('base64').replace(/=/g, '');
    const encodedPayload = Buffer.from(JSON.stringify(payload)).toString('base64').replace(/=/g, '');
  
    const signature = forge.hmac.create();
    signature.start('sha256', secrete_str);
    signature.update(`${encodedHeader}.${encodedPayload}`);
    const encodedSignature = forge.util.encode64(signature.digest().getBytes()).replace(/=/g, '');
  
    return `${encodedHeader}.${encodedPayload}.${encodedSignature}`;
  }
  
  function generateRandomString(length) {
      const bytes = forge.random.getBytesSync(Math.ceil(length / 2));
      const hexString = forge.util.bytesToHex(bytes);
      return hexString.substring(0, length);
  }
  
  function generateAccessToken(httpBody) {
  
    if (!apiCredentials?.token) {
      throw new Error("No API credentials found, extension is not connected");
    }


    const md = forge.md.sha256.create();
	md.update(httpBody);
	const bodyHash = md.digest().toHex();
    
    // Example payload
    const payload = {
      body_hash: bodyHash,
      user: 'zano_extension',
      salt: generateRandomString(64),
      exp: Math.floor(Date.now() / 1000) + (60), // Expires in 1 minute
    };
    
    return createJWSToken(payload, apiCredentials?.token);
  }


// JSON-RPC 2.0 request body
const requestData = {
  jsonrpc: '2.0',
  id: 0,
  method: 'getbalance',
  params: {},
};


const http_body = JSON.stringify(requestData);

// Create JWT token
const token = generateAccessToken(http_body);


// Send POST request
axios.post('http://127.0.0.1:11111/json_rpc', requestData, {
  headers: {
    'Content-Type': 'application/json',
    'Zano-Access-Token': `${token}`,
  },
})
.then(response => {
  console.log('Response:', response.data);
})
.catch(error => {
  if (error.response) {
    console.error('Error Response:', error.response.status, error.response.data);
  } else {
    console.error('Request Error:', error.message);
  }
});
