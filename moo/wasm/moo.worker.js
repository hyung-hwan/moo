self.importScripts('libmoo.js');
self.__ready = false;

Module.onRuntimeInitialized = function()
{
        self.__ready = true;
        console.log ('runtime is ready now...');
}

const libmoo = {
        open: Module.cwrap('moo_openstd', 'number', ['number', 'number', 'number']),
        close: Module.cwrap('moo_close', '', ['number']),
	ignite: Module.cwrap('moo_ignite', 'number', ['number', 'number']),
	initdbgi: Module.cwrap('moo_initdbgi', 'number', ['number', 'number']),
	compilefile: Module.cwrap('moo_compilefileb', 'number', ['number', 'string']),
	invoke: Module.cwrap('moo_invokebynameb', 'number', ['number', 'string', 'string'])
};

/*
var log_write = Module.addFunction(function() {

});
*/

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
                        var moo, tmp;
			var msg = "";

			moo = libmoo.open(0, null, null);
			msg = msg.concat("open - " + moo);

			tmp = libmoo.ignite(moo, 5000000);
			msg = msg.concat(" ignite - " + tmp);

			tmp = libmoo.initdbgi(moo, 102400);
			msg = msg.concat(" initdgbi - " + tmp);

			tmp = libmoo.compilefile(moo, "kernel/test-003.moo");
			msg = msg.concat(" compilefile - " + tmp);

			tmp = libmoo.invoke(moo, "MyObject", "main");
			msg = msg.concat(" invoke - " + tmp);

                        self.postMessage (msg);
			
                        libmoo.close (moo);
                }
        } 
});
