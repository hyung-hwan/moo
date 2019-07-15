//function _jsGetTime(pointer) <--- doesn't work as emcc produces this
var _jsGetTime = function(pointer)
{
	var pos = pointer / 4;
	var now = (new Date()).getTime();
	HEAP32[pos] = Math.floor(now / 1000);
	HEAP32[pos + 1] = (now % 1000) * 1000000;
}
