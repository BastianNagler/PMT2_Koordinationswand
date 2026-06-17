var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function initWebSocket() {
    console.log('Verbinde mit WebSocket...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Verbindung hergestellt');
    addLogLine("--- Verbunden mit cWall ---");
    // Request log history
    websocket.send(JSON.stringify({ action: "get_logs" }));
}

function onClose(event) {
    console.log('Verbindung getrennt. Versuche Neuverbindung...');
    addLogLine("--- Verbindung getrennt, versuche Neuverbindung... ---");
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    try {
        var myObj = JSON.parse(event.data);

        if (myObj.action) {
            if (myObj.action === "log") {
                addLogLine(myObj.message);
            } else if (myObj.action === "log_history") {
                if (myObj.logs && Array.isArray(myObj.logs)) {
                    myObj.logs.forEach(function(log) {
                        addLogLine(log);
                    });
                    addLogLine("--- Ende der Historie ---");
                }
            }
        }
    } catch (e) {
        // Not a JSON message or parse error
    }
}

function addLogLine(message) {
    var container = document.getElementById('log-container');
    if (!container) return;

    var el = document.createElement('div');
    el.className = 'log-entry';
    el.textContent = message;
    container.appendChild(el);

    var autoScroll = document.getElementById('auto-scroll');
    if (autoScroll && autoScroll.checked) {
        container.scrollTop = container.scrollHeight;
    }
}

function clearLogs() {
    var container = document.getElementById('log-container');
    if (container) {
        container.innerHTML = '';
    }
}
