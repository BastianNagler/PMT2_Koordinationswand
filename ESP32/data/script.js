/* WEB SOCKET */
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

var timerInterval;
var timeLeft = 60;

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
}

function onClose(event) {
    console.log('Verbindung getrennt. Versuche Neuverbindung...');
    setTimeout(initWebSocket, 2000);
}

// Nachricht vom ESP32 empfangen
function onMessage(event) {
    console.log("Daten empfangen:", event.data);
    try {
        var myObj = JSON.parse(event.data);

        if (myObj.action) {
            switch (myObj.action) {

                case "start game":
                    startGameUI(myObj.mode);
                    break;

                case "update counter":
                    updateScores(myObj.scoreP1, myObj.scoreP2);
                    break;

                case "end of game":
                    endGameUI(myObj);
                    break;

                default:
                    console.warn("Unbekannte Aktion:", myObj.action);
            }
        }
    } catch (e) {
        console.error("Fehler beim Parsen der JSON:", e);
    }
}

/* =======================================
   UI UPDATE FUNKTIONEN
======================================== */

function startGameUI(mode) {
    // Ansicht wechseln
    hide(document.getElementById("endofgamescreen"));
    show(document.getElementById("gamescreen"));
    hide(document.getElementById("nameInputSection"));

    // Scores zurücksetzen
    document.getElementById("scoreP1").innerHTML = "0";
    document.getElementById("scoreP2").innerHTML = "0";

    // Layout je nach Modus anpassen
    if (mode === "single") {
        document.getElementById("p1Label").innerHTML = "Score";
        hide(document.getElementById("p2Section"));
    } else {
        document.getElementById("p1Label").innerHTML = "Player 1";
        show(document.getElementById("p2Section"));
    }

    // Timer im Browser starten
    timeLeft = 60;
    document.getElementById("timerValue").innerHTML = timeLeft;
    clearInterval(timerInterval); // Alten Timer sicherheitshalber löschen
    
    timerInterval = setInterval(function() {
        timeLeft--;
        if (timeLeft >= 0) {
            document.getElementById("timerValue").innerHTML = timeLeft;
        } else {
            clearInterval(timerInterval); // Stoppen, wenn 0 erreicht
        }
    }, 1000);
}

function updateScores(p1, p2) {
    if (p1 !== undefined) document.getElementById("scoreP1").innerHTML = p1;
    if (p2 !== undefined) document.getElementById("scoreP2").innerHTML = p2;
}

function endGameUI(myObj) {
    clearInterval(timerInterval); // Timer stoppen, falls er noch lief
    
    // Ansicht wechseln
    hide(document.getElementById("gamescreen"));
    show(document.getElementById("endofgamescreen"));

    // Info für "Letztes Spiel" aktualisieren
    updateFinalScoreDisplay();

    // Highscores rendern
    if (myObj.highscoreList && Array.isArray(myObj.highscoreList)) {
        renderHighscores(myObj.highscoreList);
    }
}

function updateFinalScoreDisplay() {
    var p1Score = parseInt(document.getElementById("scoreP1").innerText) || 0;
    var p2Score = parseInt(document.getElementById("scoreP2").innerText) || 0;
    var p2Visible = document.getElementById("p2Section").style.display !== "none";
    
    var text = "";
    if (!p2Visible) {
        text = `Singleplayer: ${p1Score} Punkte`;
    } else {
        text = `Multiplayer - P1: ${p1Score} | P2: ${p2Score}<br>`;
        if (p1Score > p2Score) text += "<strong>(Player 1 gewinnt!)</strong>";
        else if (p2Score > p1Score) text += "<strong>(Player 2 gewinnt!)</strong>";
        else text += "<strong>(Unentschieden!)</strong>";
    }
    
    document.getElementById("finalScoreDisplay").innerHTML = text;
}

function renderHighscores(highscores) {
    var listContainer = document.getElementById("highscoreDisplay");
    if (!listContainer) return;

    listContainer.innerHTML = "";
    var needsName = false;

    highscores.forEach(function (player, index) {
        var listItem = document.createElement("div");
        listItem.className = "highscore-item";
        
        // Prüfen, ob das der neue, unbenannte Highscore ist
        if (player.playerName === "TrageDeinenNamenein!") {
            needsName = true;
            listItem.style.color = "#d9534f"; // Rot hervorheben
            listItem.style.fontWeight = "bold";
            listItem.innerHTML = `${index + 1} — NEUER HIGHSCORE: ${player.counterValue} pts`;
        } else {
            // Normaler Eintrag
            listItem.innerHTML = `${index + 1} — ${player.playerName}: ${player.counterValue} pts`;
        }
        listContainer.appendChild(listItem);
    });

    // Zeige das Namens-Eingabefeld an, wenn ein Name fehlt
    if (needsName) {
        show(document.getElementById("nameInputSection"));
        document.getElementById("playerNameInput").value = ""; // Feld leeren
        document.getElementById("playerNameInput").focus();
    } else {
        hide(document.getElementById("nameInputSection"));
    }
}

// Namensformular an ESP32 senden
function handlePlayerNameForm(event) {
    event.preventDefault(); // Verhindert das Neuladen der Seite
    var nameInput = document.getElementById("playerNameInput").value;

    if (nameInput && websocket && websocket.readyState === WebSocket.OPEN) {
        const data = {
            action: "set name",
            playerName: nameInput
        };
        websocket.send(JSON.stringify(data));
        
        // Feld nach dem Senden verstecken, wir warten auf die neue Liste vom ESP32
        hide(document.getElementById("nameInputSection"));
    }
}

/* =======================================
   HILFSFUNKTIONEN
======================================== */
function hide(element) {
    if (element) element.style.display = 'none';
}

function show(element) {
    if (element) element.style.display = 'flex';
}