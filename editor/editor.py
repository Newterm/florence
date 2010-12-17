#!/usr/bin/python
import sys
from xml.dom import minidom
import gtk
import toolkit

class key(toolkit.object):
	def __init__(self, code, x, y, w, h):
		toolkit.object.__init__( self, code, int((x-(w/2.0))*30.0), int((y-(h/2.0))*30.0), int(w*30.0), int(h*30.0) )

	def __str__(self):
		w = self.w / 30.0
		h = self.h / 30.0
		str = "<key>\n"
		str = str + "\t<code>%s</code>\n" % self.name
		str = str + "\t<xpos>%s</xpos>\n" % ( ( self.x + (self.w/2.0) ) / 30.0 )
		str = str + "\t<ypos>%s</ypos>\n" % ( ( self.y + (self.h/2.0) ) / 30.0 )
		if w != 2:
			str = str + "\t<width>%s</width>\n" % w
		if h != 2:
			str = str + "\t<height>%s</height>\n" % h
		str = str + "</key>\n"
		return str

class keyboard(toolkit.scene):
	def __init__(self, node=None):
		toolkit.scene.__init__( self )
		if node:
			self.load( node )
		else:
			self.setSize()
		self.setgrid( 30, 30 );

	def setSize(self, w=20*30, h=10*30):
		self.w = w
		self.h = h
		toolkit.scene.setSize( self, w, h )

	def getSize(self):
		return (self.w, self.h)
	
	def reset(self):
		toolkit.scene.reset( self )
		self.setSize()
		self.setgrid( 30, 30 );

	def load(self, node):
		w = node.getElementsByTagName('width')[0].firstChild.data
		h = node.getElementsByTagName('height')[0].firstChild.data
		w = int ( float(w) * 30.0 )
		h = int ( float(h) * 30.0 )
		toolkit.scene.reset( self )
		self.setSize( w, h )
		keys = node.getElementsByTagName('key')
		for k in keys:
			try:
				codes = k.getElementsByTagName('code')
				if len(codes) > 0:
					code = codes[0].firstChild.data
				else:
					action = k.getElementsByTagName('action')[0] 
					command = action.getElementsByTagName('command')
					if len(command) > 0:
						code = command[0].firstChild.data
					else:
						code = action.firstChild.data
			except Exception as e:
				print k.toxml()
				print e
				quit()
			x = float(k.getElementsByTagName('xpos')[0].firstChild.data)
			y = float(k.getElementsByTagName('ypos')[0].firstChild.data)
			try:
				w = float(k.getElementsByTagName('width')[0].firstChild.data)
			except IndexError:
				w = 2
			try:
				h = float(k.getElementsByTagName('height')[0].firstChild.data)
			except IndexError:
				h = 2
			self.add( key(code, x, y, w, h) )
		self.setgrid( 30, 30 );

	def __str__(self):
		str = "<keyboard>\n"
		for k in self.objects:
			str = str + k.__str__()
		str = str + "</keyboard>\n"
		return str

class extension:
	def __init__(self):
		self.x = 0
		self.y = 0

	def loadmain(self, keyboard):
		self.placement = "main"
		self.keyboard = keyboard

	def load(self, node):
		self.placement = node.getElementsByTagName('placement')[0].firstChild.data
		self.keyboard = keyboard( node.getElementsByTagName('keyboard')[0] )

	def getSize(self):
		(w, h) = self.keyboard.getSize()
		return (w/3.0, h/3.0)

	def setPos(self, x, y):
		self.x = x
		self.y = y

	def draw(self, crctx, selected):
		(w, h) = self.keyboard.getSize()
		if selected:
			crctx.set_source_rgba(0.1, 0, 0.5, 0.5)
		else:
			crctx.set_source_rgba(0, 0, 0, 0.5)
		crctx.rectangle(self.x, self.y, w/3.0, h/3.0)
		crctx.fill()
		crctx.rectangle(self.x, self.y, w/3.0, h/3.0)
		crctx.set_source_rgba(0, 0, 0, 1)
		crctx.set_line_width(1)
		crctx.stroke()

	def select(self, x, y):
		(w, h) = self.keyboard.getSize()
		x2 = self.x + ( w/3.0 )
		y2 = self.y + ( h/3.0 )
		return ( x > self.x ) and ( y > self.y ) and ( x < x2 ) and ( y < y2 )

class extensions:
	def __init__(self, keyboard):
		self.exts = []
		self.main = extension()
		self.main.loadmain( keyboard )
		self.select = self.main

	def reset(self, keyboard):
		self.exts = []
		self.main = extension()
		self.main.loadmain( keyboard )

	def load(self, widget, nodes):
		self.exts = []
		for ext in nodes:
			local = extension()
			local.load( ext )
			self.exts.append( local )

		tops = []
		bots = []
		lefts = []
		rights = []
		for ext in self.exts:
			if ext.placement == "top":
				tops.append(ext)
			elif ext.placement == "bottom":
				bots.append(ext)
			elif ext.placement == "left":
				lefts.append(ext)
			elif ext.placement == "right":
				rights.append(ext)
			else:
				print "Extension placement error"

		x = 0
		y = 0
		ww = 0
		hh = 0
		for ext in lefts:
			(w, h) = ext.getSize()
			x += w
		for ext in tops:
			ext.setPos(x, y)
			(w, h) = ext.getSize()
			y += h
			hh += h
		x = 0
		for ext in lefts:
			ext.setPos(x, y)
			(w, h) = ext.getSize()
			x += w
			ww += w
		(w, h) = self.main.getSize()
		self.main.setPos(x, y)
		x += w
		ww += w
		hh += h
		for ext in rights:
			ext.setPos(x, y)
			(w, h) = ext.getSize()
			x += w
			ww += w
		x = 0
		for ext in bots:
			ext.setPos(x, y)
			(w, h) = ext.getSize()
			y += h
			hh += h

		widget.connect("expose-event", self.expose, self)
		widget.set_size_request(int(ww), int(hh))
		widget.queue_draw()

	def expose(self, widget, event, data):
		crctx = widget.window.cairo_create()
		crctx.set_source_rgba(1, 1, 1, 1)
		crctx.paint()
		self.main.draw(crctx, self.select == self.main)
		for ext in self.exts:
			ext.draw(crctx, self.select == ext)
	
	def setSel(self, x, y):
		sel = None
		if self.main.select(x, y):
			sel = self.main
		else:
			for ext in self.exts:
				if ext.select(x, y):
					sel = ext
					break
		if sel:
			self.select = sel
		return self.select.keyboard

class editor:
	def __init__(self):
		self.exts = None
		self.file = None
		self.builder = gtk.Builder()
		self.builder.add_from_file("editor.glade")

		self.builder.get_object("keyboard").set_events(gtk.gdk.ALL_EVENTS_MASK)
		self.builder.get_object("exts").set_events(gtk.gdk.ALL_EVENTS_MASK)

		self.kbd = keyboard()
		if len( sys.argv ) > 1:
			self.file = sys.argv[1]
			self.load( self.file )
		else:
			self.new( None )
			self.exts = extensions(self.kbd)
		self.kbd.connect( self.builder.get_object("keyboard") )

		self.builder.connect_signals(self)
		self.builder.get_object("editor").show()
		self.builder.get_object("extensions").show()
		self.builder.get_object("exts").connect("button-press-event", self.selectExtension, self)

	def load( self, file ):
		self.kbd.reset()
		xmldoc = minidom.parse(file)
		self.kbd.load(xmldoc.getElementsByTagName('keyboard')[0])
		(w, h) = self.kbd.getSize()
		self.builder.get_object("keyboard").set_size_request(w, h)
		if self.exts:
			self.exts.reset(self.kbd)
		else:
			self.exts = extensions(self.kbd)
		nodes = xmldoc.getElementsByTagName('extension')
		self.exts.load(self.builder.get_object("exts"), nodes)

	def new( self, widget ):
		self.kbd.reset()
		if self.exts:
			self.exts.reset(self.kbd)
		else:
			self.exts = extensions( self.kbd )
		(w, h) = self.kbd.getSize()
		self.builder.get_object("keyboard").set_size_request(w, h)

	def open( self, widget ):
		chooser = gtk.FileChooserDialog( title="Open layout file", action=gtk.FILE_CHOOSER_ACTION_OPEN,
				buttons=( gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK))
		#TODO: use $PREFIX
		chooser.set_current_folder("/usr/share/florence/layouts")
		if self.file:
			chooser.set_filename( self.file )
		filter = gtk.FileFilter()
		filter.set_name( "Layout files (*.xml)" )
		filter.add_pattern( "*.xml" )
		chooser.add_filter( filter )
		response = chooser.run()
		if response == gtk.RESPONSE_OK:
			self.load( chooser.get_filename() )
		chooser.destroy()

	def gg( self, widget ):
		#print self.kbd
		gtk.main_quit()

	def selectExtension(self, widget, event, data):
		kbd = self.exts.setSel(event.x, event.y)
		if kbd:
			self.kbd.disconnect( self.builder.get_object("keyboard") )
			self.kbd = kbd
			self.kbd.connect( self.builder.get_object("keyboard") )
			(w, h) = self.kbd.getSize()
			self.builder.get_object("keyboard").set_size_request(int(w), int(h))
			self.builder.get_object("keyboard").queue_draw()
			widget.queue_draw()

ed = editor()
gtk.main()
