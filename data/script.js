// WebSocket setup
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

let countdownTimer;
let countdownDisplay = document.createElement('div');

window.addEventListener('load', onLoad);

// Function to start the 10-second countdown
function startCountdown() {
    let countdownValue = 10;
    countdownDisplay.innerText = countdownValue + " seconds remaining";
    countdownDisplay.className = 'countdown-timer';
    document.querySelector('.log-container').appendChild(countdownDisplay);
    
    // Decrement countdown value every second
    countdownTimer = setInterval(() => {
      countdownValue--;
      countdownDisplay.innerText = countdownValue + " seconds remaining";
      if (countdownValue <= 0) {
        clearInterval(countdownTimer);  // Stop timer
        removeCountdown();
      }
    }, 1000);
}
  
  // Function to remove the countdown timer
function removeCountdown() {
    clearInterval(countdownTimer);
    if (countdownDisplay) {
      countdownDisplay.remove();
    }
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  console.log("new_readings", event.data);

  let message = JSON.parse(event.data);

  // Remove timer and add new button when IR data is saved
  if (message.status === "saved") {
      // Remove the countdown timer
      removeCountdown();

      // Check if a button with the same name already exists
      let buttonContainer = document.getElementById("button-container");
      let existingButton = Array.from(buttonContainer.children).find(button => button.innerText === message.name);
      
      if (!existingButton) {
          // Create a new button with the saved name
          let newButton = document.createElement("button");
          newButton.className = "icon-btn";
          newButton.innerText = message.name;

          newButton.onclick = () => {
              websocket.send(JSON.stringify({ Command: "on", Name: message.name }));
          };

          buttonContainer.appendChild(newButton);
      } else {
          console.log(`Button "${message.name}" already exists.`);
      }
  }
}


function onLoad(event) {
    initWebSocket();
}
// Send data via WebSocket
function sendMessage(data) {
    if (websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify(data));
    }
}

document.getElementById("start-btn").addEventListener("click", () => {
    let irName = document.getElementById("ir-name").value;
    if (irName) {
      let message = JSON.stringify({ Command: "start", Name: irName });
      console.log(message);
      websocket.send(message);
      startCountdown();
    }
  });

document.getElementById("clear-btn").addEventListener("click", () => {
  let message = JSON.stringify({ Command: "reset" });
  websocket.send(message);
});

  // Toggle Power
function togglePower() {
  let powerBtn = document.getElementById("power-btn");
  let powerIcon = document.getElementById("power-icon");
  let powerStatus = powerBtn.classList.contains("on") ? "off" : "on";

  powerBtn.classList.toggle("on");
  powerBtn.classList.toggle("off");

  powerBtn.innerHTML = powerStatus === "on" ? "<i class='fas fa-power-off'></i> On" : "<i class='fas fa-power-off'></i> Off";

  sendMessage({ power: powerStatus });
}

// Adjust Temperature
function adjustTemperature(change) {
  let tempInput = document.getElementById("temperature-input");
  let currentTemp = parseInt(tempInput.value);
  let newTemp = currentTemp + change;
  
  if (newTemp >= 16 && newTemp <= 30) {
      tempInput.value = newTemp;
      sendMessage({ temperature: newTemp });
  }
}

// Toggle Swing
function toggleSwing(type) {
  let swingBtn = document.getElementById(`swing-${type}`);
  let swingStatus = swingBtn.classList.contains("on") ? "off" : "on";

  swingBtn.classList.toggle("on");
  swingBtn.classList.toggle("off");

  swingBtn.style.backgroundColor = swingStatus === "on" ? "#4CAF50" : "#ddd";
  sendMessage({ [`swing_${type}`]: swingStatus });
}

// Set Fan Speed
function setFanSpeed(speed) {
  let speeds = ["low", "medium", "high" , "auto"];
  speeds.forEach(function(s) {
      let btn = document.getElementById(`fan-${s}`);
      btn.classList.remove("selected");
  });

  let selectedBtn = document.getElementById(`fan-${speed}`);
  selectedBtn.classList.add("selected");

  sendMessage({ fan_speed: speed });
}

// Send data via WebSocket
function sendMessage(data) {
  if (websocket.readyState === WebSocket.OPEN) {
      websocket.send(JSON.stringify(data));
  }
}

// Update the UI based on the fetched AC status
function updateUIWithStatus(status) {
  // Power status
  const powerButton = document.getElementById("power-btn");
  const powerIcon = document.getElementById("power-icon");
  if (status.hasOwnProperty("power"))
  {
    if (status.power === "on") {
        powerButton.classList.remove('off');
        powerButton.classList.add('on');
        powerIcon.classList.remove('fa-power-off');
        powerIcon.classList.add('fa-power-on');
        powerButton.innerText = "On";
    } else {
        powerButton.classList.remove('on');
        powerButton.classList.add('off');
        powerIcon.classList.remove('fa-power-on');
        powerIcon.classList.add('fa-power-off');
        powerButton.innerText = "Off";
    }
  }
  if (status.hasOwnProperty("temperature"))
  {
    const temperatureInput = document.getElementById("temperature-input");
    temperatureInput.value = status.temperature !== undefined ? status.temperature : "24"; // Default to 24 if not available
  }
  if (status.hasOwnProperty("fan_speed"))
  {
    console.log(status.fan_speed);
    const fanButtons = document.querySelectorAll(".speed-btn");
    fanButtons.forEach(button => button.classList.remove('active')); // Remove active class
    if (status.fan_speed) {
        const activeButton = document.getElementById(`fan-${status.fan_speed}`);
        if (activeButton) {
            activeButton.classList.add('active'); // Mark the active fan speed button
        }
    }
  }
  if (status.hasOwnProperty("swing_horizontal"))
  {
    const swingHorizontal = document.getElementById("swing-horizontal");
    swingHorizontal.classList.remove('active');
    if (status.swing_horizontal === "on") 
    {
      swingHorizontal.classList.add('active');
    }
  }
  if (status.hasOwnProperty("swing_vertical"))
  {
    const swingVertical = document.getElementById("swing-vertical");  
    swingVertical.classList.remove('active');
    if (status.swing_vertical === "on") {
        swingVertical.classList.add('active');
    }
  }
}

function fetchACStatus() {
  fetch('/acstatus')
      .then(response => response.json())
      .then(data => {
          // Update the UI with the fetched status
          console.log(data);
          updateUIWithStatus(data);
      })
      .catch(error => {
          console.error('Error fetching AC status:', error);
      });
}
// Fetch the JSON from the /manuals route when the page loads
window.onload = function() {
  fetch('/manuals')
    .then(response => response.json())
    .then(data => {
      // For each key-value pair in the fetched JSON, create a button
      for (let buttonName in data) {
        if (data.hasOwnProperty(buttonName)) {
          let buttonContainer = document.getElementById("button-container");
          
          // Create a new button for each item in the JSON
          let newButton = document.createElement("button");
          newButton.className = "icon-btn";
          newButton.innerText = buttonName;  // Button name from the JSON key

          // When the button is clicked, send the corresponding command
          newButton.onclick = () => {
            websocket.send(JSON.stringify({ Command: "on", Name: buttonName }));
          };

          buttonContainer.appendChild(newButton);
        }
      }
    })
    .catch(error => {
      console.error('Error fetching button data:', error);
    });

  fetchACStatus();
};