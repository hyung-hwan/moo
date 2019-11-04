self.importScripts('libmoo.js');
self.libmoo = null;
self.moo = null;

Module.noExitRuntime = false;
Module.onExit = function (status) {
	console.log ('exiting....');
};

Module.onRuntimeInitialized = function()
{
	console.log ('runtime is ready now...');

	self.libmoo = {
		open: Module.cwrap('moo_openstd', 'number', ['number', 'number', 'number']),
		close: Module.cwrap('moo_close', '', ['number']),
		ignite: Module.cwrap('moo_ignite', 'number', ['number', 'number']),
		initdbgi: Module.cwrap('moo_initdbgi', 'number', ['number', 'number']),
		compilefile: Module.cwrap('moo_compilefileb', 'number', ['number', 'string']),
		invoke: Module.cwrap('moo_invokebynameb', 'number', ['number', 'string', 'string']),
		geterrmsg: Module.cwrap('get_errmsg_from_moo', 'string', ['number']),
		switchprocess: Module.cwrap('switch_process_in_moo', 'undefined', ['number'])
	};
};

//self.onmessage = function (evt) {
self.addEventListener ('message', function (evt) {
	var objData = evt.data;

	var cmd = objData.cmd;
	if (cmd === "run-moo")
	{
		 //var x = Module.ccall('moo_openstd', [0, null, null]);
		 //Module.ccall('moo_close', x);

		if (self.moo !== null)
		{
			var tmp, msg = "";
			//var ticker;

			// while self.libmoo.invoke() is running, setInterval() is unable to trigger the timed event.
			// it's an architectural issue. emscripten doesn't implement setitimer() yet. 
			// the moo program should 'yield' manually for process switching for now.
			//ticker = setInterval(function() { self.libmoo.switchprocess (self.moo); console.log("ticking"); }, 100);

			tmp = self.libmoo.invoke(self.moo, "MyObject", "main");
			msg = msg.concat("invoke - " + tmp);

			//clearInterval (ticker);

			self.postMessage (msg);
		}
		else
		{
			self.postMessage ("not open");
		}
	} 
	else if (cmd == "open-moo")
	{
		if (self.libmoo === null)
		{
			self.postMessage ("not ready");
		}
		else if (self.moo === null)
		{
			var tmp, msg = "";

			self.moo = self.libmoo.open(0, null, null);
			msg = msg.concat("open - " + moo);

			tmp = self.libmoo.ignite(self.moo, 5000000);
			msg = msg.concat(" ignite - " + tmp);

			tmp = self.libmoo.initdbgi(self.moo, 102400);
			msg = msg.concat(" initdgbi - " + tmp);

			tmp = self.libmoo.compilefile(self.moo, "kernel/t.moo");
			if (tmp == -1)
			{
				msg = msg.concat(" compilefile - " + self.libmoo.geterrmsg(self.moo));
			}
			else
			{
				msg = msg.concat(" compilefile - " + tmp);
			}

			self.postMessage (msg);
		}
		else
		{
			self.postMessage ("already open");
		}
	}

	else if (cmd == "close-moo")
	{
		if (self.moo !== null) 
		{
			self.libmoo.close (self.moo);
			self.moo = null;
			self.postMessage ("close ok");
		}
		else
		{
			self.postMessage ("already closed");
		}
	}
});
