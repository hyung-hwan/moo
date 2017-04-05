#include 'Moo.moo'.

###var xxxx. ## global variables.

class X11(Object) from 'x11'
{
	dcl(#class) connection.
	dcl id.

	dcl event_loop_sem event_loop_proc.
	dcl ll_event_blocks.

	dcl expose_event.
	dcl key_event.
	dcl mouse_event.
	dcl mouse_wheel_event.

	dcl frames.

	method(#primitive) _connect.
	method(#primitive) _disconnect.
	method(#primitive) _get_fd.
	method(#primitive) _get_event.

	method id { ^self.id }
}

class X11.Exception(System.Exception)
{
}

## ---------------------------------------------------------------------------
## Event
## ---------------------------------------------------------------------------
pooldic X11.LLEvent
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
	CLIENT_MESSAGE    := 33.
}

class X11.Event(Object)
{
}

class X11.InputEvent(X11.Event)
{
}

class X11.KeyEvent(X11.InputEvent)
{
}

class X11.MouseEvent(X11.InputEvent)
{
	dcl x y.

	method x { ^self.x }
	method y { ^self.y }

	method x: x y: y
	{
		self.x := x.
		self.y := y.
	}
}

class X11.MouseWheelEvent(X11.InputEvent)
{
	dcl x y amount.
	
	method x { ^self.x }
	method y { ^self.y }
	method amount { ^self.amount }

	method x: x y: y amount: amount
	{
		self.x := x.
		self.y := y.
		self.amount := amount.
	}
}

class X11.ExposeEvent(X11.Event)
{
	dcl x y width height.

	method x { ^self.x }
	method y { ^self.y }
	method width { ^self.width }
	method height { ^self.height }
	
	method x: x y: y width: width height: height
	{
		self.x := x.
		self.y := y.
		self.width := width.
		self.height := height.
	}
}

class X11.WindowEvent(X11.Event)
{
}

class X11.FrameEvent(X11.Event)
{
}


## ---------------------------------------------------------------------------
## Graphics Context
## ---------------------------------------------------------------------------
class X11.GC(Object) from 'x11.gc'
{
	dcl window id.

	method(#primitive) _make (connection, window).
	method(#primitive) _kill.
	method(#primitive) _drawLine(x1, y1, x2, y2).
	method(#primitive) _drawRect(x, y, width, height).
	method(#primitive) _fillRect(x, y, width, height).
	method(#primitive) _foreground: color.
	
	method(#class) new: window
	{
		^(super new) __open_on: window
	}

	method __open_on: window
	{
		if ((id := self _make(window connection id, window id)) isError) 
		{
			X11.Exception signal: 'cannot make a graphic context'
		}.

		self.window := window.
		self.id := id.
	}

	method close
	{
		if (self.window notNil)
		{
			self _kill.
			self.window := nil.
		}
	}
}

## ---------------------------------------------------------------------------
## ---------------------------------------------------------------------------
class X11.Component(Object)
{
	dcl parent.

	method parent
	{
		^self.parent
	}

	method parent: parent
	{
		self.parent := parent.
	}
}

class X11.Canvas(Component)
{
	method paint: gr
	{
		self subclassResponsibility: #paint
	}

	method update: gr
	{
		self subclassResponsibility: #paint
	}
}

class X11.Container(Component)
{
	dcl components capacity size.

	method initialize
	{
		self.size := 0.
		self.capacity := 128.
## TODO: change System.Array to Danamically resized array.
		self.components := System.Array new: self.capacity.
	}

	method addComponent: component
	{
		| tmp |
		component parent: self.
		if (self.size >= self.capacity)
		{
			tmp := System.Array new: (self.capacity + 128).
			tmp copy: self.components.
			self.capacity := self.capacity + 128.
			self.components := tmp.
		}.
		
		self.components at: self.size put: component.
		self.size := self.size + 1.
	}
}

class X11.Window(Container) from 'x11.win'
{
	dcl id.

	method(#primitive) _get_dwatom.
	method(#primitive) _get_id.
	
	method(#primitive) _make (connection, x, y, width, height, parent).
	method(#primitive) _kill.

	method id { ^self.id }

	method connection 
	{ 
		| p pp |
		p := self.
		while ((pp := p parent) notNil) { p := pp }.
		^p connection.
	}

	method(#class) new: parent
	{
		^(super new) __open_on_window: parent
	}
	
	method __open_on_window: parent
	{
		| id |
		id := self _make (parent connection id, 5, 5, 100, 100, parent id).
		if (id isError) { X11.Exception signal: ('cannot make a window - ' & id asString) }.
		
		self.id := id.
		self.parent := parent.
		parent addComponent: self.

		parent connection addFrame: self.
		
		self windowOpened.
	}

	method expose: event
	{
		('EXPOSE  IN WINDOWS....' & (self.id asString) & ' ' & (event x asString) & ' ' & (event y asString) & ' ' & (event width asString) & ' ' & (event height asString))  dump.
	}

	(* TODO: take the followings out to a seperate mouse listener??? *)
	method mouseEntered: event
	{
	}
	method mouseExited: event
	{
	}
	method mouseClicked: event
	{
	}
	method mousePressed: event
	{
		('MOUSE PRESSED' & (self.id asString) & ' ' & (event x asString) & ' ' & (event y asString))  dump.
	}
	method mouseReleased: event
	{
		('MOUSE RELEASED' & (self.id asString) & ' ' & (event x asString) & ' ' & (event y asString))  dump.
	}
	method mouseWheelMoved: event
	{
		('MOUSE WHEEL MOVED' & (self.id asString) & ' ' & (event x asString) & ' ' & (event y asString) &   ' ' & (event amount asString))  dump.
	}


	method windowOpened
	{
	}
	method windowClosing
	{
	}
	method windowClosed
	{
	}

}

class X11.Frame(Window)
{
	dcl connection.

	## method connection: connection { self.connection := connection } 
	method connection { ^self.connection }
	
	method(#class) new
	{
		##^(super new) __open_on: X11 connection.
		^(super new) __open_on: X11 connect.  ## connect if X11 is not connected.
	}

	method __open_on: connection
	{
		| id |

		id := self _make(connection id, 10, 20, 500, 600, nil).
		if (id isError) { X11.Exception signal: ('cannot make a frame - ' & id asString) }.		

		self.id := id.
		self.connection := connection.

		connection addFrame: self.
		
		self windowOpened.
	}

	method close
	{
		if (self.connection notNil)
		{
			self windowClosing.
			self _kill.
			(* TODO: remove all subcomponents... *)
			self.connection removeFrame: self.
			self windowClosed.
			
			self.id := 0.
			self.connection := nil.
		}
	}

	##method show: flag
	##{
	##}

	(*
	method windowActivated: event
	{
	}
	method windowDeactivated: event
	{
	}
	method windowIconified: event
	{
	}
	method windowDeiconified: event
	{
	}*)
}

class X11.Button(Component)
{
}


extend X11
{
	method(#class) connect
	{
		| c | 
		if (self.connection isNil) 
		{
			c := self new.
			if (c connect isError) { X11.Exception signal: 'cannot connect' }.
			self.connection := c.
		}.
		^self.connection
	}
	
	method(#class) disconnect
	{
		if (self.connection notNil) 
		{
			self.connection disconnect.
			self.connection := nil.
		}.
		^self.connection
	}
	
	method(#class) connection
	{
		^self.connection
	}

	method(#class) enterEventLoop
	{
		^self connect enterEventLoop
	}

	method initialize
	{
		super initialize.

		self.expose_event := X11.ExposeEvent new.
		self.key_event := X11.KeyEvent new.
		self.mouse_event := X11.MouseEvent new.
		self.mouse_wheel_event := X11.MouseWheelEvent new.

		self.ll_event_blocks := System.Dictionary new.
		self.ll_event_blocks
			at: X11.LLEvent.KEY_PRESS      put: #__handle_key_event:;
			at: X11.LLEvent.KEY_RELEASE    put: #__handle_key_event:;
			at: X11.LLEvent.BUTTON_PRESS   put: #__handle_button_event:;
			at: X11.LLEvent.BUTTON_RELEASE put: #__handle_button_event:;
			at: X11.LLEvent.MOTION_NOTIFY  put: #__handle_notify:;
			at: X11.LLEvent.ENTER_NOTIFY   put: #__handle_notify:;
			at: X11.LLEvent.LEAVE_NOTIFY   put: #__handle_notify:;
			at: X11.LLEvent.EXPOSE         put: #__handle_expose:;
			at: X11.LLEvent.CLIENT_MESSAGE put: #__handle_client_message:.
	}

	method connect
	{
		| t |
		if (self.frames isNil)
		{
			if ((t := self _connect) isError)  { ^t }.
			self.id := t.
			self.frames := System.Dictionary new.
		}
	}

	method disconnect
	{
		if (self.frames notNil)
		{
			self.frames do: [ :frame |
				frame close.
			].
			self.frames := nil.
			self _disconnect.
		}
	}
	
	method addFrame: frame
	{
		if (frame connection ~= self)
		{
			X11.Exception signal: 'Invalid connection in frame to add'.
		}.

('ADDING NE FRAME ID' & frame id asString) dump.
		self.frames at: (frame id) put: frame.
	}

	method removeFrame: frame
	{
		if (frame connection ~= self)
		{
			X11.Exception signal: 'Invalid connection in frame to remove'.
		}.

		(*self.frames removeLink: frame backlink.
		frame connection: nil.
		frame backlink: nil.*)

('REMOVING NE FRAME ID' & frame id asString) dump.
		(self.frames removeKey: (frame id)) dump.
	}

	method enterEventLoop
	{
		if (self.event_loop_sem isNil)
		{
			self.event_loop_sem := Semaphore new.
			Processor signal: self.event_loop_sem onInput: (self _get_fd).
			self.event_loop_proc := [ 
				| event ongoing | 
				
				ongoing := true.
				while (self.frames size > 0) 
				{
'Waiting for X11 event...' dump.
					self.event_loop_sem wait.
					if (ongoing not) { break }.

					while ((event := self _get_event) notNil) 
					{
					## XCB_EXPOSE 12
					## XCB_DESTROY_WINDOW 4
('EVENT================>' & event asString) dump.
						if (event isError)
						{
							 'Error has occurred in ....' dump.
							ongoing := false.
							break.
						}
						else 
						{ 
							self __dispatch_event: event. 
						}.
					}.
				}.

				Processor unsignal: self.event_loop_sem.
				self.event_loop_sem := nil.
				
				self disconnect.
			] fork.
		}
	}
	
	method exitEventLoop
	{
		if (self.event_loop_sem notNil)
		{
			###self.even_loop_sem ...
			self.event_loop_proc terminate.
			self.event_loop_proc := nil.
			self.event_loop_sem := nil.
		}
	}

	method signal_event_loop_semaphore
	{
		self.event_loop_sem signal.
	}

	method __dispatch_event: event
	{
		| type mthname |
		
		##type := System _getUint8(event, 0).
		type := event getUint8(0).
		
		mthname := self.ll_event_blocks at: (type bitAnd: 16r7F).
		if (mthname isError)
		{
			(* unknown event. ignore it *)
('IGNORING UNKNOWN LL-EVENT TYPE ' & type asString) dump.
		}
		else
		{
('Performing ...' & mthname asString) dump.
			^self perform (mthname, event).
			##^self perform: mthname with: event.
		}
	}

	method __handle_notify: type
	{
		^9999999999
	}

	method __handle_expose: event
	{
		(*
			typedef struct xcb_expose_event_t {
				uint8_t      response_type;
				uint8_t      pad0;
				uint16_t     sequence;
				xcb_window_t window; // uint32_t
				uint16_t     x;
				uint16_t     y;
				uint16_t     width;
				uint16_t     height;
				uint16_t     count;
				uint8_t      pad1[2];
			} xcb_expose_event_t;
		*)

		| wid frame |

		##wid := System _getUint32(event, 4). ## window
		wid := event getUint32(4).
		frame := self.frames at: wid.

		if (frame notError)
		{
			(*
			self.expose_event 
				x: System _getUint16(event, 8)        ## x
				y: System _getUint16(event, 10)       ## y
				width: System _getUint16(event, 12)   ## width
				height: System _getUint16(event, 14). ## height
			*)
			self.expose_event 
				x: event getUint16(8)        ## x
				y: event getUint16(10)       ## y
				width: event getUint16(12)   ## width
				height: event getUint16(14). ## height

			frame expose: self.expose_event.
		}
		else
		{
			System logNl: ('Expose event on unknown window - ' & wid asString).
		}
	}

	method __handle_button_event: event
	{
		(*
			typedef struct xcb_button_press_event_t {
				uint8_t         response_type;
				xcb_button_t    detail;  // uint8_t
				uint16_t        sequence;
				xcb_timestamp_t time;    // uint32_t
				xcb_window_t    root;    // uint32_t
				xcb_window_t    event;
				xcb_window_t    child;
				int16_t         root_x;
				int16_t         root_y;
				int16_t         event_x;
				int16_t         event_y;
				uint16_t        state;
				uint8_t         same_screen;
				uint8_t         pad0;
			} xcb_button_press_event_t;
			typedef xcb_button_press_event_t xcb_button_release_event_t;
		*)

		| type wid frame detail |
		type := System _getUint8(event, 0) bitAnd: 16r7F. ## lower 7 bits of response_type
		wid := System _getUint32(event, 12). ## event
		##type := event getUint8(0) bitAnd: 16r7F. ## lower 7 bits of response_type
		##wid := event getUint32(12). ## event
		
		frame := self.frames at: wid.

		if (frame notError)
		{
			##detail := System _getUint8(event, 1).
			detail := event getUint8(1).
			if (detail >= 1 and: [detail <= 3])
			{
				(*
				self.mouse_event 
					## TODO: encode detail also..
					x: System _getUint16(event, 24)   ## event_x
					y: System _getUint16(event, 26).  ## event_y
				*)
				self.mouse_event 
					## TODO: encode detail also..
					x: event getUint16(24)   ## event_x
					y: event getUint16(26).  ## event_y
				
				if (type == X11.LLEvent.BUTTON_PRESS)
				{
					frame mousePressed: self.mouse_event
				}
				else
				{
					frame mouseReleased: self.mouse_event
				}
			}
			elsif (detail == 4 or: [detail == 5])
			{
				if (type == X11.LLEvent.BUTTON_RELEASE)
				{
					(*
					self.mouse_wheel_event
						x: System _getUint16(event, 24)   ## event_x
						y: System _getUint16(event, 26)   ## event_y
						amount: (if (detail == 4) { -1 } else { 1 }).
					*)
					self.mouse_wheel_event
						x: event getUint16(24)   ## event_x
						y: event getUint16(26)   ## event_y
						amount: (if (detail == 4) { -1 } else { 1 }).
					frame mouseWheelMoved: self.mouse_wheel_event
				}
			}
		}
		else
		{
			System logNl: ('Button event on unknown window - ' & wid asString).
		}
	}
	
	method __handle_client_message: event
	{
		(*
			typedef union xcb_client_message_data_t {
				uint8_t  data8[20];
				uint16_t data16[10];
				uint32_t data32[5];
			} xcb_client_message_data_t;

			typedef struct xcb_client_message_event_t {
				uint8_t                   response_type;
				uint8_t                   format;
				uint16_t                  sequence;
				xcb_window_t              window;    // uint32_t
				xcb_atom_t                type;      // uint32_t
				xcb_client_message_data_t data;
			} xcb_client_message_event_t;
		*)
		| type wid frame dw |
		
		##wid := System _getUint32(event, 4). ## window
		wid := event getUint32(4). ## window
		frame := self.frames at: wid.
		
		if (frame notError)
		{
			##dw := System _getUint32(event, 12). ## data.data32[0]
			dw := event getUint32(12). ## data.data32[0]
			if (dw == frame _get_dwatom)
			{
				## TODO: call close query callback???
				frame close.
			}
			
			## TODO: handle other client messages
		}
		else
		{
			System logNl: ('Client message on unknown window - ' & wid asString).
		}
	}

	method __handle_key_event: type
	{
		(*
			typedef struct xcb_key_press_event_t {
				uint8_t         response_type;
				xcb_keycode_t   detail;
				uint16_t        sequence;
				xcb_timestamp_t time;
				xcb_window_t    root;
				xcb_window_t    event;
				xcb_window_t    child;
				int16_t         root_x;
				int16_t         root_y;
				int16_t         event_x;
				int16_t         event_y;
				uint16_t        state;
				uint8_t         same_screen;
				uint8_t         pad0;
			} xcb_key_press_event_t;
			typedef xcb_key_press_event_t xcb_key_release_event_t;
		*)

		if (type = X11.LLEvent.KEY_PRESS)
		{
		}
		else
		{
		}
	}
}


class MyButton(X11.Window)
{
	method expose: event
	{
	|gc|
	

		super expose: event.
		

gc := X11.GC new: self.	
gc _foreground: 2.	
gc _fillRect(0, 0, 50, 50).
gc close.
	}
}

class MyFrame(X11.Frame)
{
	dcl gc.
	dcl b1.
	dcl b2.

	method windowOpened
	{
		super windowOpened.
		if (self.gc isNil)
		{
			self.gc := X11.GC new: self.
self.gc _foreground: 10.
self.gc _drawLine(10, 20, 30, 40).
self.gc _drawRect(10, 20, 30, 40).
self.gc _foreground: 20.
self.gc _drawRect(100, 100, 200, 200).
		}.
		
		b1 := MyButton new: self.
	}

	method windowClosing
	{
		super windowClosing.
		(*if (self.gc notNil)
		{
			self.gc close.
			self.gc := nil.
		}*)
	}

	method expose: event
	{
		super expose: event.
		
		(*
		('EXPOSE....' & (self.id asString) & ' ' & (event x asString) & ' ' & (event y asString) & ' ' & (event width asString) & ' ' & (event height asString))  dump.

self.gc _foreground: 2.	
##self.gc drawLine: (10@20) to: (30@40).
self.gc _drawLine(10, 20, 300, 400).
self.gc _drawRect(10, 20, 30, 40).
self.gc _foreground: 20.
self.gc _drawRect(100, 100, 200, 200).*)
	}
}



class MyObject(Object)
{	
	method(#class) abc
	{
	^9234554123489129038123908123908312908
	}
	
	method(#class) main 
	{
		| f q |

		f := 20.
		(q:=:{ 10 -> 20, 20 -> 30, f + 40 -> self abc, (Association key: 10 value: 49), f -> f })dump.
		(q at: 60) dump.
		(q at: 10) dump.


		#( #a:y: #b #c) dump.
		f := LinkedList new.
		f addLast: 10.
		f addLast: 20.
		q := f addLast: 30.
		f addLast: 40.
		f removeLink: q.
		f addLastLink: q.
		(f findLink: 30) prev value dump.


		f do: [:v | v dump ].
		(f size asString & ' elements in list') dump.

		##X11 connect.
	##	f := MyFrame new.
		q := MyFrame new.

	##	MyButton new: q.
	##	MyButton new: f.

(*
		f add: X11.Button new: 'click me'.
		f width: 200 height: 200.
		f show.
*)
		
		X11 enterEventLoop. ## this is not a blocking call. it spawns another process.

		(*while (true)
		{
			'111' dump.
			##x signal_event_loop_semaphore.
			Processor sleepFor: 5.
		}.*)
		## [ while (true) { '111' dump. Processor sleepFor: 1. } ] fork.

		##X11 disconnect.
	}
}
