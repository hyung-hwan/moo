#include 'Moo.moo'.

class X11(Object) from 'x11'
{
	## =====================================================================
	## this part of the class must match the internal
	## definition struct x11_t  defined in _x11.h
	## ---------------------------------------------------------------------
	var display_base := nil.
	var expose_event.
	var key_event.
	var mouse_event.
	var mouse_wheel_event.
	## =====================================================================

	var shell_container := nil.
	var window_registrar. ## all windows registered

	var event_loop_sem, event_loop_proc.
	var llevent_blocks.

	method(#primitive,#liberal) _open_display(name).
	method(#primitive) _close_display.
	method(#primitive) _get_fd.
	method(#primitive) _get_llevent(llevent).

	method(#primitive) _create_window(parent_window, x, y, width, height, fgcolor, bgcolor).
	method(#primitive) _destroy_window(window).

	##method(#primitive) _fill_rectangle(window, gc, x, y, width, height).
	method(#primitive) _draw_rectangle(window, gc, x, y, width, height).

	method __create_window(parent_window, x, y, width, height, fgcolor, bgcolor, owner)
	{
		| w |
		w := self _create_window(parent_window, x, y, width, height, fgcolor, bgcolor).
		if (w notError) { self.window_registrar at: w put: owner }.
		^w
	}

	method __close_window(window_handle)
	{
		| w |
		w := self _destroy_window(window_handle).
		if (w notError) { self.window_registrar removeKey: window_handle }
	}
}

class X11.Exception(System.Exception)
{
}

class X11.Point(Object)
{
	var(#get,#set) x := 0, y := 0.
}

class X11.Dimension(Object)
{
	var(#get,#set) width := 0, height := 0.
}

class X11.Rectangle(Object)
{
	var(#get,#set)
		x      := 0,
		y      := 0,
		width  := 0,
		height := 0.
}

## ---------------------------------------------------------------------------
## Event
## ---------------------------------------------------------------------------
pooldic X11.LLEventType
{
	KEY_PRESS         := 2.
	KEY_RELEASE       := 3.
	BUTTON_PRESS      := 4.
	BUTTON_RELEASE    := 5.
	MOTION_NOTIFY     := 6.
	ENTER_NOTIFY      := 7.
	LEAVE_NOTIFY      := 8.
	EXPOSE            := 12.
	VISIBILITY_NOTIFY := 15.
	DESTROY_NOTIFY    := 17.
	CONFIGURE_NOTIFY  := 22.
	CLIENT_MESSAGE    := 33.
}

class X11.LLEvent(Object)
{
	var(#get) type := 0, window := 0, x := 0, y := 0, width := 0, height := 0.
}

class X11.Event(Object)
{
}


class X11.KeyEvent(X11.Event)
{
}

pooldic X11.MouseButton
{
	LEFT := 1.
	MIDDLE := 2.
	RIGHT := 3.
}

class X11.MouseEvent(X11.Event)
{
	var(#get,#set)
		x      := 0,
		y      := 0,
		button := 0. ## X11.MouseButton
}

class X11.MouseWheelEvent(X11.Event)
{
	var(#get,#set)
		x      := 0,
		y      := 0,
		amount := 0.
}

class X11.ExposeEvent(X11.Event)
{
	var(#get,#set)
		x      := 0,
		y      := 0,
		width  := 0,
		height := 0.
}



## ---------------------------------------------------------------------------
## X11 Context
## ---------------------------------------------------------------------------
class X11.GraphicsContext(Object)
{
	var(#get) widget := nil, gcHandle := nil.

	var(#get,#set)
		foreground := 0,
		background := 0,
		lineWidth  := 1,
		lineStyle  := 0,
		fillStyle  := 0.

	method(#class) new: widget
	{
		^(super new) __make_gc_on: widget
	}

	method __make_gc_on: widget
	{
		| gc |
		gc := widget displayServer _create_gc (widget windowHandle).
		if (gc isError) { selfns.Exception signal: 'Cannot create a graphics context' }.
		self.gcHandle := gc.
		self.widget := widget.
	}

	method fillRectangle(x, y, width, height)
	{
		^self.widget displayServer _fill_rectangle (self.widget windowHandle, self.gcHandle, x, y, width, height).
	}

	method drawRectangle(x, y, width, height)
	{
		^self.widget displayServer _draw_rectangle (self.widget windowHandle, self.gcHandle, x, y, width, height).
	}
}

## ---------------------------------------------------------------------------
## X11 Widgets
## ---------------------------------------------------------------------------

class X11.Widget(Object)
{
	var(#get,#set)
		parent   := nil,
		x        := 0,
		y        := 0,
		width    := 0,
		height   := 0,
		fgcolor  := 0,
		bgcolor  := 0,
		realized := false.


	method displayServer
	{
		if (self.parent isNil) { ^nil }.
		^self.parent displayServer.
	}

	method windowHandle
	{
		(* i assume that a widget doesn't' with a low-level window by default.
		 * if a widget maps to a window, the widget class must implement the
		 * windowHandle method to return the low-level window handle.
		 * if it doesn't map to a window, it piggybacks on the parent's window *)
		if (self.parent isNil) { ^nil }.
		^self.parent windowHandle.
	}

	method paint: paint_event
	{
	}

	method realize
	{
	## super realize chaining required???
	}

	method dispose
	{
	## what should be done first? remvoe from container? should dispose be called?
	## super dispose chaining required?
		##if (self.parent notNil)
		##{
		##	self.parent remove: self.
		##}
	}
}

class X11.Label(X11.Widget)
{
	var(#get) text := ''.

	method text: text
	{
		self.text := text.
		if (self windowHandle notNil) { self paint: nil }
	}

	method paint: paint_event
	{
		| gc |
System logNl: 'LABEL GC.......'.
		gc := selfns.GraphicsContext new: self.
		gc foreground: 100.
		gc drawRectangle (self.x, self.y, self.width, self.height).
		###gc.drawText (self.title)
	}

	method realize
	{
		## if i want to use a window to represent it, it must create window here.
		## otherwise, do other works in paint:???
	}

	method dispose
	{

	}
}

class X11.Composite(X11.Widget)
{
	var children.

	method initialize
	{
		self.children := LinkedList new.
	}

	method add: widget
	{
		if (widget parent notNil)
		{
			selfns.Exception signal: 'Cannot add an already added widget'.
		}.

		self.children addLast: widget.
		widget parent: self.
	}

	method remove: widget
	{
		| link |
		if (widget parent =~ self)
		{
			selfns.Exception sinal: 'Cannot remove an unknown widget'
		}.

		## TODO: unmap and destroy...
		link := self.children findLink: widget.
		self.children removeLink: link.
		widget parent: nil.
	}

	method childrenCount
	{
		^self.children size
	}

	method dispose
	{
		self.children do: [:child | child dispose ]
	}
}

class X11.Shell(X11.Composite)
{
	var(#get) title := ''.
	var(#get,#set) displayServer := nil.
	var(#get) windowHandle := nil.

	method new: title
	{
		self.title := title.
	}

	method title: title
	{
		self.title := title.
		if (self.windowHandle notNil)
		{
			## set window title of this window.
		}
	}

	method realize
	{
		| wind |
		if (self.windowHandle notNil)  { ^self }.

		wind := self.displayServer __create_window(nil, self.x, self.y, self.width, self.height, self.fgcolor, self.bgcolor, self).
		if (wind isError)
		{
			self.Exception signal: ('Cannot create shell ' & self.title).
		}.

		self.windowHandle := wind.

		[
			self.children do: [:child | child realize ].
		] on: System.Exception do: [:ex |
			self.displayServer _destroy_window(wind).
			self.windowHandle := nil.
			ex pass
		].

### call displayOn: from the exposure handler...
self paint: nil.
	}

	method dispose
	{
		super dispose.
		if (self.windowHandle notNil)
		{
			self.displayServer _destroy_window (self.windowHandle).
			self.windowHandle := nil.
		}
	}
}

## ---------------------------------------------------------------------------
## X11 server
## ---------------------------------------------------------------------------
extend X11
{
	method(#class) new
	{
		^(super new) __connect_to: nil.
	}

	method __connect_to: name
	{
		if (self _open_display(name) isError)
		{
			self.Exception signal: 'cannot open display'
		}
.
		self.display_base dump.
	}

	method dispose
	{
		if (self.shell_container notNil)
		{
			self.shell_container dispose.
			self.shell_container := nil.
		}.

		if (self.display_base notNil)
		{
			self _close_display.
			##self.display_base := nil.
		}.

		## TODO: check if _fini_trailer is not called for some exception throwing...
		##self _fini_trailer.
	}

	method initialize
	{
		super initialize.

		self.expose_event := self.ExposeEvent new.
		self.key_event := self.KeyEvent new.
		self.mouse_event := self.MouseEvent new.
		self.mouse_wheel_event := self.MouseWheelEvent new.

		self.shell_container := self.Composite new.
		self.window_registrar := System.Dictionary new: 100.

		(*

		self.llevent_blocks := System.Dictionary new.
		self.llevent_blocks
			at: self.LLEventType.KEY_PRESS         put: #__handle_key_event:;
			at: self.LLEventType.KEY_RELEASE       put: #__handle_key_event:;
			at: self.LLEventType.BUTTON_PRESS      put: #__handle_button_event:;
			at: self.LLEventType.BUTTON_RELEASE    put: #__handle_button_event:;
			at: self.LLEventType.MOTION_NOTIFY     put: #__handle_notify:;
			at: self.LLEventType.ENTER_NOTIFY      put: #__handle_notify:;
			at: self.LLEventType.LEAVE_NOTIFY      put: #__handle_notify:;
			at: self.LLEventType.EXPOSE            put: #__handle_expose:;
			at: self.LLEventType.DESTROY_NOTIFY    put: #__handle_destroy_notify:;
			at: self.LLEventType.CONFIGURE_NOTIFY  put: #__handle_configure_notify:;
			at: self.LLEventType.CLIENT_MESSAGE    put: #__handle_client_message:.
		*)
		self.llevent_blocks := %{
			self.LLEventType.KEY_PRESS         -> #__handle_key_event:,
			self.LLEventType.KEY_RELEASE       -> #__handle_key_event:,
			self.LLEventType.BUTTON_PRESS      -> #__handle_button_event:,
			self.LLEventType.BUTTON_RELEASE    -> #__handle_button_event:,
			self.LLEventType.MOTION_NOTIFY     -> #__handle_notify:,
			self.LLEventType.ENTER_NOTIFY      -> #__handle_notify:,
			self.LLEventType.LEAVE_NOTIFY      -> #__handle_notify:,
			self.LLEventType.EXPOSE            -> #__handle_expose:,
			self.LLEventType.DESTROY_NOTIFY    -> #__handle_destroy_notify:,
			self.LLEventType.CONFIGURE_NOTIFY  -> #__handle_configure_notify:,
			self.LLEventType.CLIENT_MESSAGE    -> #__handle_client_message:
		}.

	}

	method addShell: shell
	{
		if (shell displayServer isNil)
		{
			self.shell_container add: shell.
			shell displayServer: self.
		}
	}

	method removeShell: shell
	{
		if (shell displayServer notNil)
		{
			self.shell_container remove: shell.
			shell displayServer: nil.
		}
	}


	method enterEventLoop
	{
		if (self.event_loop_sem isNil)
		{
			self.event_loop_sem := Semaphore new.
			Processor signal: self.event_loop_sem onInput: (self _get_fd).
			self.event_loop_proc := [
				| llevtbuf llevent ongoing |

				llevtbuf := X11.LLEvent new.
				ongoing := true.
				while (self.shell_container childrenCount > 0)
				{
'Waiting for X11 event...' dump.
					self.event_loop_sem wait.
					if (ongoing not) { break }.

					while ((llevent := self _get_llevent(llevtbuf)) notNil)
					{
						if (llevent isError)
						{
							##System logNl: ('Error while getting a event from server ' & self.cid asString).
							ongoing := false.
							break.
						}
						else
						{
							self __dispatch_llevent: llevent.
						}.
					}.
				}.

				Processor unsignal: self.event_loop_sem.
				self.event_loop_sem := nil.

				self dispose.
'CLOSING X11 EVENT LOOP' dump.
			] fork.
		}
	}

	method exitEventLoop
	{
		if (self.event_loop_sem notNil)
		{
			self.event_loop_proc terminate.
			self.event_loop_proc := nil.
			self.event_loop_sem := nil.
		}
	}

	method signal_event_loop_semaphore
	{
		self.event_loop_sem signal.
	}

	method __dispatch_llevent: llevent
	{
		| widget mthname |

		widget := self.window_registrar at: llevent window.
		if (widget isError)
		{
			System logNl: 'Event on unknown widget - ' & (llevent window asString).
			^nil
		}.

		mthname := self.llevent_blocks at: llevent type.
		if (mthname isError)
		{
			System logNl: 'Uknown event type ' & (llevent type asString).
			^nil
		}.

(llevent window asString) dump.
		^self perform (mthname, llevent).
	}

	method __handle_notify: type
	{
		^9999999999
	}

	method __handle_expose: event
	{
	}

	method __handle_button_event: event
	{
	}

	method __handle_destroy_notify: event
	{
	}

	method __handle_configure_notify: event
	{
	}

	method __handle_client_message: event
	{
	}

	method __handle_key_event: type
	{
	}
}




class MyObject(Object)
{
	var disp1, shell1, shell2.

	method main1
	{
		self.disp1 := X11 new.

		shell1 := (X11.Shell new title: 'Shell 1').
		shell2 := (X11.Shell new title: 'Shell 2').

		shell1 x: 10; y: 20; width: 100; height: 100.
		shell2 x: 200; y: 200; width: 200; height: 200.

		self.disp1 addShell: shell1.
		self.disp1 addShell: shell2.

		self.shell1 add: (X11.Label new text: 'xxxxxxxx').
		self.shell1 realize.
		self.shell2 realize.

		self.disp1 enterEventLoop. ## this is not a blocking call. it spawns another process.
	}

	method(#class) main
	{
		^self new main1
	}
}

