import './style.css'

var __info = {};
var __availableFW = {};
var __mqtt = {};
var __wifi = {};
const emptypassword = "#$%__EMPTY__%$#";

function setElementHtml(name, html) {
  var elem = document.getElementById(name);
  elem.innerHTML = html;
}

function setElementValue(name, value) {
  var elem = document.getElementById(name);
  elem.value = value;
}

function getElementValue(name) {
  var elem = document.getElementById(name);
  return elem.value;
}

function setCheckbox(name, value) {
  var elem = document.getElementById(name);
  elem.checked = value;
}
function getCheckbox(name) {
  var elem = document.getElementById(name);
  return elem.checked;
}

function addEvent(id, event, handler) {
  var elem = document.getElementById(id);
  elem.addEventListener(event, handler);
}

function setDisableValue(id, value) {
  var elem = document.getElementById(id);
  if (value)
    elem.setAttribute("disabled", "disabled");
  else
    elem.removeAttribute("disabled");
}

function json(url) {
  return fetch(url).then(function (res) { return res.json() })
}

function post(url, body) {
  var headers = new Headers();
  headers.append('Content-Type', 'application/json');

  return fetch(url, {
    method: 'POST',
    mode: 'same-origin',
    credentials: 'include',
    redirect: 'follow',
    headers: headers,
    body: body ? JSON.stringify(body) : null,
  }).then(function (res) { return res.json() });
}

function getInfo() {
  json("/info").then(function (json) {
    __info = json;
    setElementValue("name", json.name);
    setElementHtml("id", json.id);
    setElementHtml("version", json.fw_version);
    setElementValue("updateServer", json.update_server);
    setCheckbox("autoUpdate", json.auto_update);

    setDisableValue("saveDevice", false);
    setDisableValue("restartDevice", false);
    if (json.update_server && json.update_server.length) {
      setDisableValue("checkFW", false);
      checkNewVersion();
    }
  }).catch(function (err) {
    console.log(err);
  });
}

function getMQTT() {
  json("mqtt").then(function (json) {
    __mqtt = json;
    setElementValue("mqtt_server", json.server);
    setElementValue("mqtt_in_topic", json.in_topic);
    setElementValue("mqtt_out_topic", json.out_topic);
    setCheckbox("mqtt_active", json.active);

    if (json.server && json.server.length) {
      setDisableValue("reconnectMQTT", false);
      setDisableValue("testMQTT", false);
      setDisableValue("saveMQTT", false);
    }
  });
}

function getWifi() {
  json("/wifi").then(function (json) {
    var list = document.getElementById("wifi_list");
    list.innerHTML = "";
    __wifi = json;
    json.networks.forEach(function (f) {
      var li = document.createElement("li")
      list.appendChild(li);
      li.innerHTML = f.ssid + "(" + f.signal + ")";
      li.addEventListener("click", function () { selectWifi(f.ssid) });
    });
  });
}

function selectWifi(ssid) {
  var network = __wifi.networks.find(function (f) { return f.ssid === ssid; })
  setElementHtml("wifi_ssid", ssid);
  setElementValue("wifi_password", network.saved ? emptypassword : "");

  var forget = document.getElementById("forgetWifi");
  if (network.saved)
    forget.removeAttribute("disabled");
  else
    forget.setAttribute("disabled", "disabled");

}

/* Handlers */

function checkNewVersion() {
  var url = __info.update_server;
  if (url.substr(-1) != "/")
    url += "/";
  url += __info.code + ".json";
  json(url).then(function (json) {
    __availableFW = json;
    setElementHtml("availableVersion", json.version);
    var el = document.getElementById("version");
    var hasnewversion = __info.fw_date < json.date;
    el.classList.remove(hasnewversion ? "up-to-date" : "new-version");
    el.classList.add(hasnewversion ? "new-version" : "up-to-date");
    setDisableValue("updateFW", !hasnewversion);
  });
}
function updateFirmware() {
  var url = __info.update_server;
  if (url.substr(-1) != "/")
    url += "/";
  url += __info.code + ".bin";
  post("/update", { file: url })
}
function restart() {
  post("/restart");
}
function saveDeviceInfo() {
  __info.name = getElementValue("name");
  if (__info.name.length == 0) {
    __info.name = __info.code + "-" + __info.id;
    setElementValue("name", __info.name);
  }
  __info.update_server = getElementValue("updateServer");
  __info.auto_update = getCheckbox("autoUpdate");

  post("/info/update", __info).then(function (json) {
  });
}
function reconnectMQTT() {
  post("/mqtt/reconnect").then(function (json) {
  });
}
function testMQTT() {
  post("/mqtt/text").then(function (json) {
  });
}
function saveMQTT() {
  __mqtt.server = getElementValue("mqtt_server");

  __mqtt.in_topic = getElementValue("mqtt_in_topic");
  if (__mqtt.in_topic.length == 0) {
    __mqtt.in_topic = __info.code + "/" + __info.id + "/in";
    setElementValue("mqtt_in_topic", __mqtt.in_topic);
  }

  __mqtt.out_topic = getElementValue("mqtt_out_topic");
  if (__mqtt.out_topic.length == 0) {
    __mqtt.out_topic = __info.code + "/" + __info.id + "/out";
    setElementValue("mqtt_out_topic", __mqtt.out_topic);
  }

  __mqtt.active = getCheckbox("mqtt_active");
  post("/mqtt", __mqtt).then(function (json) {
  });
}
function refreshWifi() {
  console.log("refresh Wifi");
}
function connectWifi() {
  console.log("connect Wifi");
}
function forgetWifi() {
  console.log("forgetWifi");
}

function updateServeKeyUp(evt) {
  setDisableValue("checkFW", evt.target.value.length == 0);
  setDisableValue("updateFW", true);
}

function mqttServeKeyUp(evt) {
  var disable = evt.target.value.length == 0;
  setDisableValue("reconnectMQTT", disable);
  setDisableValue("testMQTT", disable);
}


document.body.onload = function () {
  document.body.removeAttribute("cloak");

  addEvent("checkFW", "click", checkNewVersion);
  addEvent("updateFW", "click", updateFirmware);
  addEvent("restartDevice", "click", restart);
  addEvent("saveDevice", "click", saveDeviceInfo);
  addEvent("reconnectMQTT", "click", reconnectMQTT);
  addEvent("testMQTT", "click", testMQTT);
  addEvent("saveMQTT", "click", saveMQTT);
  addEvent("refreshWifi", "click", refreshWifi);
  addEvent("connectWifi", "click", connectWifi);
  addEvent("forgetWifi", "click", forgetWifi);

  addEvent("updateServer", "keyup", updateServeKeyUp);
  addEvent("mqtt_server", "keyup", mqttServeKeyUp);

  /* buttons start disabled */
  setDisableValue("checkFW", true);
  setDisableValue("updateFW", true);
  setDisableValue("restartDevice", true);
  setDisableValue("saveDevice", true);
  setDisableValue("reconnectMQTT", true);
  setDisableValue("testMQTT", true);
  setDisableValue("saveMQTT", true);
  setDisableValue("refreshWifi", true);
  setDisableValue("connectWifi", true);
  setDisableValue("forgetWifi", true);

  getInfo();
  getMQTT();
  getWifi();
};