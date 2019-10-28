import './style.css'
import axios from 'axios';

function setElementHtml(name, html) {
  var elem = document.getElementById(name);
  elem.innerHTML = html;
}

function setElementValue(name, value) {
  var elem = document.getElementById(name);
  elem.value = value;
}

function setCheckbox(name, value) {
  var elem = document.getElementById(name);
  elem.checked = value;
}

function getInfo() {
  setElementValue("name", "MAX");
  setElementHtml("version", "0.0.0");
  setElementValue("updateServer", "http://update.me");
  setElementHtml("availableVersion", "1.0.0");
}

function getMQTT() {
  setElementValue("mqtt_server", "http://my.mqtt.broker.url");
  setElementValue("mqtt_topic", "mqtt_topic");
  setCheckbox("mqtt_active", 1);
}


document.body.onload = function () {
  document.body.removeAttribute("cloak");
  getInfo();
  getMQTT();
};