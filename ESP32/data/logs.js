var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function initWebSocket() {
    console.log('Connect to WebSocket...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection established');
    addLogLine("--- Connected to cWall ---");
    // Request log history
    websocket.send(JSON.stringify({ action: "get_logs" }));
}

function onClose(event) {
    console.log('Connection closed. Attempting to reconnect...');
    addLogLine("--- Connection closed, attempting to reconnect... ---");
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
                    clearLogs();
                    addLogLine("--- Connected to cWall ---");
                    myObj.logs.forEach(function(log) {
                        addLogLine(log);
                    });
                    addLogLine("--- End of History ---");
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
