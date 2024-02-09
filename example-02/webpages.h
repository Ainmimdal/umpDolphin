const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>UMP Dolphin</title>
  <style>
       body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background-color: #f4f4f4;
    }
    h1 {
      margin-bottom: 20px;
      color: #333;
    }
    p {
      color: #666;
    }
    button {
      background-color: #007bff;
      color: #fff;
      border: none;
      padding: 8px 12px;
      cursor: pointer;
      border-radius: 5px;
      transition: background-color 0.3s ease;
    }
    button:hover {
      background-color: #0056b3;
    }
    ul.file-list {
      list-style-type: none;
      padding: 0;
      margin-top: 20px;
      border-top: 1px solid #ddd;
    }
    li.file-item {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 10px 0;
      border-bottom: 1px solid #ddd;
    }
    .file-actions {
      display: flex;
      gap: 10px;
    }
    .download-btn,
    .delete-btn {
      background-color: #28a745;
      color: #fff;
      border: none;
      padding: 5px 10px;
      cursor: pointer;
      border-radius: 5px;
      transition: background-color 0.3s ease;
    }
    .delete-btn {
      background-color: #dc3545;
    }
    .download-btn:hover,
    .delete-btn:hover {
      background-color: #1f7d38;
    }
  </style>
</head>
<body>
  <h1>UMP Dolphin</h1>
  <p>Main page</p>
  <p>Firmware: %FIRMWARE%</p>
  <p>Free Storage: <span id="freespiffs">%FREESPIFFS%</span> | Used Storage: <span id="usedspiffs">%USEDSPIFFS%</span> | Total Storage: <span id="totalspiffs">%TOTALSPIFFS%</span></p>
  <p>
    <button onclick="logoutButton()">Logout</button>
    <button onclick="rebootButton()">Reboot</button>
    <button onclick="listFilesButton()">List Files</button>
    <button onclick="showUploadButtonFancy()">Upload File</button>
  </p>
  <p id="status"></p>
  <div id="detailsheader"></div>
  <ul id="details" class="file-list"></ul>
  <script>
    function logoutButton() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/logout", true);
      xhr.send();
      setTimeout(function() {
        window.open("/logged-out", "_self");
      }, 1000);
    }

    function rebootButton() {
     
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/reboot", true);
      xhr.send();
      window.open("/reboot", "_self");
    }

    function listFilesButton() {
      var xmlhttp = new XMLHttpRequest();
      xmlhttp.open("GET", "/listfiles", false);
      xmlhttp.send();
      document.getElementById("detailsheader").innerHTML = "<h3>Files</h3>";
      document.getElementById("details").innerHTML = xmlhttp.responseText;
    }

    function downloadDeleteButton(filename, action) {
      var urltocall = "/file?name=" + filename + "&action=" + action;
      var xmlhttp = new XMLHttpRequest();
      if (action == "delete") {
        xmlhttp.open("GET", urltocall, false);
        xmlhttp.send();
        document.getElementById("status").innerHTML = xmlhttp.responseText;
        xmlhttp.open("GET", "/listfiles", false);
        xmlhttp.send();
        document.getElementById("details").innerHTML = xmlhttp.responseText;
      }
      if (action == "download") {
        document.getElementById("status").innerHTML = "";
        window.open(urltocall, "_blank");
      }
    }

    function showUploadButtonFancy() {
      document.getElementById("detailsheader").innerHTML = "<h3>Upload File</h3>";
      document.getElementById("status").innerHTML = "";
      var uploadform = '<form method="POST" action="/" enctype="multipart/form-data"><input type="file" name="data"/><input type="submit" name="upload" value="Upload" title="Upload File"></form>';
      document.getElementById("details").innerHTML = uploadform;
      var uploadform = '<form id="upload_form" enctype="multipart/form-data" method="post">' +
        '<input type="file" name="file1" id="file1" onchange="uploadFile()"><br>' +
        '<progress id="progressBar" value="0" max="100" style="width:300px;"></progress>' +
        '<h3 id="status"></h3>' +
        '<p id="loaded_n_total"></p>' +
        '</form>';
      document.getElementById("details").innerHTML = uploadform;
    }

    function _(el) {
      return document.getElementById(el);
    }

    function uploadFile() {
      var file = _("file1").files[0];
      var formdata = new FormData();
      formdata.append("file1", file);
      var ajax = new XMLHttpRequest();
      ajax.upload.addEventListener("progress", progressHandler, false);
      ajax.addEventListener("load", completeHandler, false);
      ajax.addEventListener("error", errorHandler, false);
      ajax.addEventListener("abort", abortHandler, false);
      ajax.open("POST", "/");
      ajax.send(formdata);
    }

    function progressHandler(event) {
      _("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes";
      var percent = (event.loaded / event.total) * 100;
      _("progressBar").value = Math.round(percent);
      _("status").innerHTML = Math.round(percent) + "% uploaded... please wait";
      if (percent >= 100) {
        _("status").innerHTML = "Please wait, writing file to filesystem";
      }
    }

    function completeHandler(event) {
      _("status").innerHTML = "Upload Complete";
      _("progressBar").value = 0;
      var xmlhttp = new XMLHttpRequest();
      xmlhttp.open("GET", "/listfiles", false);
      xmlhttp.send();
      document.getElementById("status").innerHTML = "File Uploaded";
      document.getElementById("detailsheader").innerHTML = "<h3>Files</h3>";
      document.getElementById("details").innerHTML = xmlhttp.responseText;
    }

    function errorHandler(event) {
      _("status").innerHTML = "Upload Failed";
    }

    function abortHandler(event) {
      _("status").innerHTML = "Upload Aborted";
    }
  </script>
</body>
</html>
)rawliteral";


const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>Logged Out</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background-color: #f4f4f4;
    }
    p {
      color: #666;
    }
    a {
      color: #007bff;
      text-decoration: none;
    }
    a:hover {
      text-decoration: underline;
    }
  </style>
</head>
<body>
  <p><a href="/">Log Back In</a></p>
</body>
</html>
)rawliteral";

const char reboot_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>Rebooting</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background-color: #f4f4f4;
    }
    h3 {
      color: #333;
    }
    #countdown {
      color: #007bff;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <h3>
    Rebooting, returning to the main page in <span id="countdown">30</span> seconds
  </h3>
  <script>
    var seconds = 30;
    function countdown() {
      seconds = seconds - 1;
      if (seconds < 0) {
        window.location = "/";
      } else {
        document.getElementById("countdown").innerHTML = seconds;
        setTimeout(countdown, 1000);
      }
    }
    countdown();
  </script>
</body>
</html>
)rawliteral";