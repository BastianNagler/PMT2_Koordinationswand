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
            if (myObj.colorStartAnim) document.getElementById('colorStartAnim').value = myObj.colorStartAnim;
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
            colorStartAnim: document.getElementById('colorStartAnim').value,
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

        setTimeout(function () {
            btn.classList.remove('success');
            text.innerHTML = 'Speichern';
        }, 2000);
    }
}

function showResetConfirm() {
    document.getElementById('resetBtn').style.display = 'none';
    document.getElementById('resetConfirmField').style.display = 'block';
}

function confirmReset(isYes) {
    if (isYes) {
        if (websocket && websocket.readyState === WebSocket.OPEN) {
            websocket.send(JSON.stringify({ action: "reset_highscore" }));

            var resetBtn = document.getElementById('resetBtn');
            var confirmField = document.getElementById('resetConfirmField');

            confirmField.style.display = 'none';
            resetBtn.style.display = 'block';

            var oldText = resetBtn.innerText;
            resetBtn.innerText = "✓ Gelöscht!";
            resetBtn.style.backgroundColor = "#2ed573";
            resetBtn.style.color = "white";
            resetBtn.style.borderColor = "#2ed573";

            setTimeout(function () {
                resetBtn.innerText = oldText;
                resetBtn.style.backgroundColor = "";
                resetBtn.style.color = "";
                resetBtn.style.borderColor = "";
            }, 2000);
        }
    } else {
        document.getElementById('resetConfirmField').style.display = 'none';
        document.getElementById('resetBtn').style.display = 'block';
    }
}
