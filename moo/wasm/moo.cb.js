//function _jsGetTime(pointer) <--- doesn't work as emcc produces this
var _jsGetTime = function(pointer)
{
	var pos = pointer / 4;
	var now = (new Date()).getTime();
	HEAP32[pos] = Math.floor(now / 1000);
	HEAP32[pos + 1] = (now % 1000) * 1000000;
}

var _jsSleep = function(pointer)
{
	/*
	var pos = pointer / 4;
	var msec = (HEAP32[POS] * 1000) + (HEAP32[pos + 1] / 1000000);

	var prom = new Promise(function (resolve, reject) {
		setTimeout (function() { resolve("xxxx"); }, msec);
	});	

	await prom;
	*/
}

