const express = require('express');
const path = require('path');
const childProcess = require('child_process');

const app = express();
const port = 3000;

app.use(express.json());
app.use(express.static(path.join(process.cwd(), 'public')));

app.post('/api/move', (req, res) => {
    const payload = req.body;
    // FIX #9: Resolve relative to __dirname so the path is stable regardless
    // of which directory the server process was started from.
    const enginePath = process.platform === 'win32'
        ? path.join(__dirname, 'Stockcaro.exe')
        : path.join(__dirname, 'Stockcaro');

    const child = childProcess.spawn(enginePath, []);

    let outputData = '';
    let errorData  = '';

    child.stdout.on('data', (data) => {
        outputData += data.toString();
    });

    child.stderr.on('data', (data) => {
        errorData += data.toString();
    });

    child.on('close', (code) => {
        if (code !== 0 && outputData.length === 0) {
            return res.status(500).json({
                error: 'Engine execution failed',
                details: errorData
            });
        }

        try {
            const result = JSON.parse(outputData);
            res.json(result);
        } catch (err) {
            // FIX #14: Do not leak raw engine output (may contain internals)
            res.status(500).json({ error: 'Failed to parse engine response' });
        }
    });

    child.stdin.write(JSON.stringify(payload) + '\n');
    child.stdin.end();
});

app.listen(port, () => {
    console.log('Stockcaro Web Server listening on http://localhost:' + port);
});
