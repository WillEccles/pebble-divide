//
// Will Eccles 2015
// Configuration Interface
// V1.0
// JS
//

// URL to configuration page. Since this is subject to change, I made it easier to change.
var configURL = "http://willeccles.github.io/dividesettings/";

var BGColor = 0;
var TColor = 0;
var BColor = 0;

// this will be used to get a color from its hex representation

// Event listener for when the pebble wants to show the configuration
Pebble.addEventListener('showConfiguration', function(e) {
	// show config page
	Pebble.openURL(configURL);
});

// get the hex value of a color string:
// #FFFFFF -> 0xFFFFFF
function hex(hexColor) {
	return parseInt(hexColor.replace("#", ""), 16);
}

Pebble.addEventListener('webviewclosed',
	function(e) {
		console.log('Configuration window returned: ' + decodeURIComponent(JSON.stringify(e)));
		var colors = decodeURIComponent(e.response).split("|");
		// set all the colors' variables
		BGColor = colors[1];
		TColor = colors[3];
		BColor = colors[5];
		
		console.log(hex(BGColor.toString()) + " " + hex(TColor.toString()) + " " + hex(BColor.toString()));
		
		// now we store all of the values in the public app keys
		var transactionId = Pebble.sendAppMessage(
			// the dictionary to send
			{
				'0': hex(BGColor.toString()),
				'1': hex(TColor.toString()),
				'2': hex(BColor.toString())
			}, 
			function(f) {console.log("Info sent to pebble successfully. TransactionID = " + f.data.transactionId);}, 
			function(f) {console.error("There was an error sending the data to the pebble. TransactionID = " + f.data.transactionId);
						console.error("Error: " + f.error.message);}
		);
		console.log("Transaction ID = " + transactionId);
	}
);