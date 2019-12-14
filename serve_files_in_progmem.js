var fs = require("fs");
var path = require("path");

function getFiles(dir, files_) {
  files_ = files_ || [];
  var files = fs.readdirSync(dir);
  for (var i in files) {
    var name = dir + '/' + files[i];
    if (fs.statSync(name).isDirectory()) {
      getFiles(name, files_);
    } else {
      files_.push(name);
    }
  }
  return files_;
}

var myArgs = process.argv.slice(2);
if (myArgs.length < 2) {
  console.error("need input folder and output file")
  return;
}

var regist_fn = myArgs.length >= 3 ? myArgs[2] : "setupServer";

var root = myArgs[0];
var outfd = fs.openSync(myArgs[1], 'w');
fs.writeSync(outfd, "// Serve files in PROGMEM\n");

var filedata = [];
getFiles(root).forEach(function (f) {
  var thisfile = {}
  thisfile.len = 0;
  thisfile.useext = path.basename(f).indexOf("NOEXT_") < 0;
  thisfile.post = path.basename(f).indexOf("POST_") >= 0;
  thisfile.nohandler = path.basename(f).indexOf("NOHANDLER_") >= 0;
  thisfile.extention = path.extname(f);
  thisfile.variable = path.basename(f).replace(/[\-\+\.]/g, "_");
  thisfile.filename = thisfile.useext ? path.basename(f) : path.basename(f, thisfile.extention);

  thisfile.filename = thisfile.filename.replace("NOEXT_", "").replace("POST_", "").replace("NOHANDLER_", "");

  thisfile.data = "const char " + thisfile.variable + "[] PROGMEM = {";
  var fd = fs.openSync(f, "r");
  var size;
  var buffer = new Buffer(512);
  while ((size = fs.readSync(fd, buffer, 0, 512)) > 0) {
    for (var i = 0; i < size; i++)
      thisfile.data += buffer[i] + ",";
    thisfile.len += size;
  }
  thisfile.data += "0 };";

  filedata.push(thisfile);
});

filedata.forEach(function (f) {
  fs.writeSync(outfd, f.data);
  fs.writeSync(outfd, "\n");
});

let filedatawithhandler = filedata.filter(function (f) { return !f.nohandler });

if (filedatawithhandler.length) {
  fs.writeSync(outfd, "// Register server\n");
  filedata.forEach(function (f) {
    if (f.nohandler) return;
    fs.writeSync(outfd, "void handle_" + f.variable + "(ESP8266WebServer &server){\n");
    fs.writeSync(outfd, "  server.setContentLength(" + f.len + ");\n")
    fs.writeSync(outfd, "  server.send_P(200, PSTR(\"");

    var dataType = "text/plain";
    switch (f.extention) {
      case ".html": dataType = "text/html"; break;
      case ".css": dataType = "text/css"; break;
      case ".js": dataType = "application/javascript"; break;
      case ".png": dataType = "image/png"; break;
      case ".gif": dataType = "image/gif"; break;
      case ".jpg": dataType = "image/jpeg"; break;
      case ".ico": dataType = "image/x-icon"; break;
      case ".xml": dataType = "text/xml"; break;
      case ".pdf": dataType = "application/pdf"; break;
      case ".zip": dataType = "application/zip"; break;
    }
    fs.writeSync(outfd, dataType);
    fs.writeSync(outfd, "\"), " + f.variable + ", " + f.len + ");\n");
    fs.writeSync(outfd, "}\n");
  })


  fs.writeSync(outfd, `void ${regist_fn}(ESP8266WebServer &server, const char *root = NULL) {\n`);
  fs.writeSync(outfd, "  String sroot = root == NULL ? String(\"/\") : String(root);\n");
  filedatawithhandler.forEach(function (f) {
    let filename = f.filename.indexOf("NOEXT_") == 0 ? f.variable : f.filename;
    fs.writeSync(outfd, "  server.on((sroot + String(\"" + filename + "\")).c_str(), " + (f.post ? "HTTP_POST" : "HTTP_GET") + ", [&]() { handle_" + f.variable + "(server); });\n");
  });
  fs.writeSync(outfd, "}\n");
}

fs.closeSync(outfd);