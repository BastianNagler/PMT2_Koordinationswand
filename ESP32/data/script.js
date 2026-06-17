/* WEB SOCKET */
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

var timerInterval;
var timeLeft = 60;

var selectedHighscore;

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
    
    // Finalen Score vom ESP32 übernehmen, um Asynchronitäten zu vermeiden
    if (myObj.finalScoreP1 !== undefined) document.getElementById("scoreP1").innerHTML = myObj.finalScoreP1;
    if (myObj.finalScoreP2 !== undefined) document.getElementById("scoreP2").innerHTML = myObj.finalScoreP2;

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
        text = `Multiplayer<br>
                - <br>
                P1: ${p1Score} | P2: ${p2Score}<br>`;
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

    highscores.forEach(function (player, index) {
        var listItem = document.createElement("div");
        listItem.className = "highscore-item";
        listItem.onclick = function(){showEditForm(index, player.playerName);};
        
        listItem.innerHTML = `${index + 1} — ${player.playerName}: ${player.counterValue} pts`;
        
        listContainer.appendChild(listItem);

        if(player.playerName == "Insert Name"){
            showEditForm(index, player.playerName);
        }
    });

}

function showEditForm(index, oldName){
    selectedHighscore = index;

    var items = document.getElementsByClassName("highscore-item");
    for (var i = 0; i < items.length; i++) {
        items[i].classList.remove("selected");
    }
    
    if (items[index]) {
        items[index].classList.add("selected");
    }

    document.getElementById("playerNameInput").value = oldName;
    show(document.getElementById("nameInputSection"));
    document.getElementById("playerNameInput").innerHtml = oldName;
    document.getElementById("playerNameInput").focus();
}

// Namensformular an ESP32 senden
function handlePlayerNameForm(event) {
    event.preventDefault(); // Verhindert das Neuladen der Seite
    var nameInput = document.getElementById("playerNameInput").value;

    if (nameInput && websocket && websocket.readyState === WebSocket.OPEN) {
        const data = {
            action: "set name",
            index: selectedHighscore,
            playerName: nameInput
        };
        websocket.send(JSON.stringify(data));
        
        // Feld nach dem Senden verstecken, wir warten auf die neue Liste vom ESP32
        hide(document.getElementById("nameInputSection"));

        var items = document.getElementsByClassName("highscore-item");
        for (var i = 0; i < items.length; i++) {
            items[i].classList.remove("selected");
        }
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