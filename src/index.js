import './style.css'
import axios from 'axios';

var info = {};
var mqtt = {};
var wifi = {};
const emptypassword = "#$%__EMPTY__%$#";

// Add a request interceptor
axios.interceptors.request.use(function (config) {
  // Do something before request is sent
  return config;
}, function (error) {
  // Do something with request error
  return Promise.reject(error);
});

// Add a response interceptor
axios.interceptors.response.use(function (response) {
  // Any status code that lie within the range of 2xx cause this function to trigger
  // Do something with response data
  return response;
}, function (error) {
  // Any status codes that falls outside the range of 2xx cause this function to trigger
  // Do something with response error
  return Promise.reject(error);
});

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
  axios.get("info").then(function (response) {
    if (response.data) {
      info = response.data;
      setElementValue("name", info.name);
      setElementHtml("version", info.version);
      setElementValue("updateServer", info.update_server);
      setElementHtml("availableVersion", "1.0.0");
    }
  });
}

function getMQTT() {
  axios.get("mqtt").then(function (response) {
    if (response.data) {
      mqtt = response.data;
      setElementValue("mqtt_server", mqtt.server);
      setElementValue("mqtt_topic", mqtt.topic);
      setCheckbox("mqtt_active", mqtt.active);
    }
  });
}

function getWifi() {
  axios.get("wifi").then(function (response) {
    if (response.data) {
      var list = document.getElementById("wifi_list");
      list.innerHTML = "";
      wifi = response.data;
      wifi.networks.forEach(function (f) {
        var li = document.createElement("li")
        list.appendChild(li);
        li.innerHTML = f.ssid + "(" + f.signal + ")";
        li.addEventListener("click", function () { selectWifi(f.ssid) });
      });
    }
  });
}

function selectWifi(ssid) {
  var network = wifi.networks.find(function (f) { return f.ssid === ssid; })
  setElementHtml("wifi_ssid", ssid);
  setElementValue("wifi_password", network.saved ? emptypassword : "");

  var forget = document.getElementById("forgetWifi");
  if (network.saved)
    forget.removeAttribute("disabled");
  else
    forget.setAttribute("disabled", "disabled");

}

document.body.onload = function () {
  document.body.removeAttribute("cloak");
  getInfo();
  getMQTT();
  getWifi();
};