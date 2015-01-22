var weatherIcon = {
    "01d" : 0,
    "02d" : 1,
    "03d" : 2,
    "04d" : 3,
    "09d" : 4,
    "10d" : 5,
    "11d" : 6,
    "13d" : 7,
    "50d" : 8,
    "01n" : 9,
    "02n" : 10,
    "03n" : 2,
    "04n" : 3,
    "09n" : 4,
    "10n" : 5,
    "11n" : 6,
    "13n" : 7,
    "50n" : 8
};

Pebble.addEventListener("ready", function(e) {});

Pebble.addEventListener("appmessage",
    function(e) {
       getWeatherOW(e.payload);
    }
);


Pebble.addEventListener('showConfiguration',
  function(e) {
    Pebble.openURL("http://pebble.newkamikaze.com/");
  }
);

Pebble.addEventListener('webviewclosed',
  function(e) {
      //console.log(e.response);
      sendMessageToPebble(JSON.parse(e.response));
  }
);

function sendMessageToPebble(result) {
    var transactionId = Pebble.sendAppMessage(
        result,
        function(e) {},
        function(e) {});
}

function getWeatherOW(params) {
    var response;

    //console.log("updating weather");
    var req = new XMLHttpRequest();
    req.open("GET",
             "http://api.openweathermap.org/data/2.5/weather?"+
             "id="+params.W_KEY.toString()+"&unit=metric&lang=ru&type=accurate", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if (req.status == 200) {
                response = JSON.parse(req.responseText);
                var temp_c = response.main.temp-273.15;
                var icon = response.weather[0].icon;
                var name = response.name;
                sendMessageToPebble({
                    "W_TEMP": temp_c,
                    "W_ICON": weatherIcon[icon],
                    "W_CITY": name
                });
            }
        }
    };
    req.send(null);
}