var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    // Hole aktuelle Konfiguration vom ESP32
    websocket.send(JSON.stringify({ action: "get_config" }));
}

function onMessage(event) {
    try {
        var myObj = JSON.parse(event.data);
        if (myObj.action === "config_data") {
            // Werte in Formular eintragen
            if (myObj.gameDurationMs) {
                document.getElementById('gameDurationMs').value = myObj.gameDurationMs / 1000;
            }
            if (myObj.colorSinglePlayer) document.getElementById('colorSinglePlayer').value = myObj.colorSinglePlayer;
            if (myObj.colorMultiplayerIdle) document.getElementById('colorMultiplayerIdle').value = myObj.colorMultiplayerIdle;
            if (myObj.colorP1) document.getElementById('colorP1').value = myObj.colorP1;
            if (myObj.colorP2) document.getElementById('colorP2').value = myObj.colorP2;
            if (myObj.colorP1Ripple) document.getElementById('colorP1Ripple').value = myObj.colorP1Ripple;
            if (myObj.colorP2Ripple) document.getElementById('colorP2Ripple').value = myObj.colorP2Ripple;
        }
    } catch (e) {
        console.error(e);
    }
}

function saveConfig(event) {
    event.preventDefault();
    
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        var config = {
            action: "set_config",
            gameDurationMs: parseInt(document.getElementById('gameDurationMs').value) * 1000,
            colorSinglePlayer: document.getElementById('colorSinglePlayer').value,
            colorMultiplayerIdle: document.getElementById('colorMultiplayerIdle').value,
            colorP1: document.getElementById('colorP1').value,
            colorP2: document.getElementById('colorP2').value,
            colorP1Ripple: document.getElementById('colorP1Ripple').value,
            colorP2Ripple: document.getElementById('colorP2Ripple').value
        };
        
        websocket.send(JSON.stringify(config));
        
        // Success Animation
        var btn = document.getElementById('saveBtn');
        var text = document.getElementById('saveBtnText');
        
        btn.classList.add('success');
        text.innerHTML = '✓ Gespeichert!';
        
        setTimeout(function() {
            btn.classList.remove('success');
            text.innerHTML = 'Speichern';
        }, 2000);
    }
}

function resetHighscore() {
    if (confirm("Bist du sicher, dass du den 60s Highscore dauerhaft löschen möchtest? Dies kann nicht rückgängig gemacht werden!")) {
        if (websocket && websocket.readyState === WebSocket.OPEN) {
            websocket.send(JSON.stringify({ action: "reset_highscore" }));
            
            // Kleines optisches Feedback auf dem Button
            var resetBtn = document.querySelector('.reset-btn');
            if (resetBtn) {
                var oldText = resetBtn.innerText;
                resetBtn.innerText = "✓ Gelöscht!";
                setTimeout(function() {
                    resetBtn.innerText = oldText;
                }, 2000);
            }
        }
    }
}
