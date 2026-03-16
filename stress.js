const net = require('net');

// We will launch 1,000 simultaneous connections
const TOTAL_CONNECTIONS = 1024; 
let connectedCount = 0;
let responsesReceived = 0;

console.log(`Launching ${TOTAL_CONNECTIONS} concurrent TCP connections to FlashKV...`);
const startTime = Date.now();

for (let i = 0; i < TOTAL_CONNECTIONS; i++) {
    const client = new net.Socket();
    
    // Connect to your C++ server
    client.connect(6379, '127.0.0.1', () => {
        connectedCount++;
        // The millisecond we connect, blast the server with a message
        client.write(`Hello from asynchronous client ${i}!\n`);
    });

    // Listen for the "+OK\n" response
    client.on('data', (data) => {
        responsesReceived++;
        // client.destroy(); // Instantly hang up the phone
        
        if (responsesReceived === TOTAL_CONNECTIONS) {
            const timeTaken = Date.now() - startTime;
            console.log(`\n SUCCESS: Handled ${TOTAL_CONNECTIONS} connections and responses in ${timeTaken} ms!`);
            process.exit(0);
        }
    });

    client.on('error', (err) => {
        console.log(`Connection ${i} failed:`, err.message);
    });
}