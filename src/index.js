import './style.css'


var deviceTime = null;
setInterval(function () {
  if (deviceTime) {
    var time = document.getElementById("time");
    time.innerText = new Date().toLocaleString();
  }
}, 100);

function getInfo() {
  var name = document.getElementById("name");
  name.innerText = "MAX";
  deviceTime = 1;
}


document.body.onload = function () {
  document.body.removeAttribute("cloak");
  getInfo();
};