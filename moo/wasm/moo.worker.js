self.importScripts('libmoo.js');
self.__ready = false;

Module.onRuntimeInitialized = function()
{
        self.__ready = true;
        console.log ('runtime is ready now...');
}

const libmoo = {
        //open: Module.cwrap('moo_openstd', 'number', ['number', 'number', 'number']),
        //close: Module.cwrap('moo_close', '', ['number']),
        open_moo: Module.cwrap('open_moo', '', [''])
};

//console.log ("QQ %O\n", self);

//self.onmessage = function (evt) {
self.addEventListener ('message', function (evt) {
        var objData = evt.data;

        var cmd = objData.cmd;
        if (cmd === "test-moo")
        {
                //var x = Module.ccall('moo_openstd', [0, null, null]);
                //Module.ccall('moo_close', x);

                if (self.__ready)
                {
                        //var moo = libmoo.open(0, null, null);
			var moo = libmoo.open_moo();
                        self.postMessage ('XXXXXXXXXXXXXXXx - ' + moo);
			
                        //libmoo.close (moo);
                }
        } 
});
