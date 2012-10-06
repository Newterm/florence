#!/usr/bin/python
import sys, io, os, time
from xml.dom import minidom
import gtk
import toolkit

class Key(toolkit.object):
	def __init__(self, code, x, y, w, h, action=None):
		self.code = code
		self.action = action
		name = self.code
		if action:
			name = self.action
		toolkit.object.__init__( self, name,
			int((x-(w/2.0))*30.0),
			int((y-(h/2.0))*30.0),
			int(w*30.0),
			int(h*30.0) )

	def __str__(self):
		w = self.w / 30.0
		h = self.h / 30.0
		str = "<key>\n"
		if self.action:
			str = str + "\t<action>%s</action>\n" % self.action
		else:
			str = str + "\t<code>%s</code>\n" % self.code
		str = str + "\t<xpos>%s</xpos>\n" % ( ( self.x + (self.w/2.0) ) / 30.0 )
		str = str + "\t<ypos>%s</ypos>\n" % ( ( self.y + (self.h/2.0) ) / 30.0 )
		if w != 2:
			str = str + "\t<width>%s</width>\n" % w
		if h != 2:
			str = str + "\t<height>%s</height>\n" % h
		str = str + "</key>\n"
		return str

	def setCode(self, code):
		self.code = code
		self.action = None
		self.name = code

	def setAction(self, action):
		self.action = action
		self.code = None
		self.name = action

	def getCode(self):
		return self.code

	def getAction(self):
		return self.action

	@staticmethod
	def load(k):
		try:
			code = None
			action = None
			if 'code' in [ x.tagName for x in k.childNodes if isinstance(x, minidom.Element) ]:
				codes = k.getElementsByTagName('code')
				code = codes[0].firstChild.data
			else:
				action = k.getElementsByTagName('action')[0] 
				command = action.getElementsByTagName('command')
				if len(command) > 0:
					command = command[0].firstChild.data
				else:
					command = action.firstChild.data
				action = command
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
		return Key(code, x, y, w, h, action=action)

class Keyboard(toolkit.scene):
	def __init__(self, builder, node=None):
		self.builder = builder
		toolkit.scene.__init__( self )
		if node:
			self.load( node )
		else:
			self.setSize()
		self.setgrid( 30, 30 );

	def setCode(self, widget, code):
		if self.sel.active:
			self.sel.active.setCode( code )
			widget.queue_draw()

	def setAction(self, widget, action):
		if self.sel.active:
			self.sel.active.setAction( action )
			widget.queue_draw()

	def setSize(self, w=20*30, h=10*30):
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
			self.add( Key.load(k) )
		self.setgrid( 30, 30 );

	def update(self, key):
		(x, y, w, h) = key.get_extents_after_hug()
		if key.getCode() == None:
			index = 0
			for action in self.builder.get_object("actions"):
				if action[0] == key.getAction(): break
				index += 1
			self.builder.get_object("actionComboBox").set_active(index)
			self.builder.get_object("notebook").set_current_page(1)
		else:
			code = int(key.getCode())
			self.builder.get_object("code").set_value(code)
			self.builder.get_object("notebook").set_current_page(0)
		self.builder.get_object("xpos").set_value(x / 30.0)
		self.builder.get_object("ypos").set_value(y / 30.0)
		self.builder.get_object("width").set_value(w / 30.0)
		self.builder.get_object("height").set_value(h / 30.0)
		self.builder.get_object("notebook").set_sensitive(True)
		self.builder.get_object("code").set_sensitive(True)
		self.builder.get_object("xpos").set_sensitive(True)
		self.builder.get_object("ypos").set_sensitive(True)
		self.builder.get_object("width").set_sensitive(True)
		self.builder.get_object("height").set_sensitive(True)

	def updateSize( self ):
		self.builder.get_object("keyboard_width").set_value(self.w / 30.0)
		self.builder.get_object("keyboard_height").set_value(self.h / 30.0)

	def clearSel(self):
		toolkit.scene.clearSel(self)
		self.builder.get_object("code").set_value(0.0)
		self.builder.get_object("xpos").set_value(0.0)
		self.builder.get_object("ypos").set_value(0.0)
		self.builder.get_object("width").set_value(2.0)
		self.builder.get_object("height").set_value(2.0)
		self.builder.get_object("notebook").set_current_page(0)
		self.builder.get_object("notebook").set_sensitive(False)
		self.builder.get_object("code").set_sensitive(False)
		self.builder.get_object("xpos").set_sensitive(False)
		self.builder.get_object("ypos").set_sensitive(False)
		self.builder.get_object("width").set_sensitive(False)
		self.builder.get_object("height").set_sensitive(False)

	def objectMenu(self, event):
		self.builder.get_object("keyMenu").popup(None, None, None, event.button, event.time) 

	def sceneMenu(self, event):
		self.builder.get_object("keyboardMenu").popup(None, None, None, event.button, event.time) 

	def __str__(self):
		str = "<keyboard>\n"
		str = str + "<width>%s</width>\n" %  ( self.w/30.0 )
		str = str + "<height>%s</height>\n" %  ( self.h/30.0 )
		for k in self.objects:
			str = str + k.__str__()
		str = str + "</keyboard>\n"
		return str

class Extension:
	def __init__(self, builder):
		self.builder = builder
		self.x = 0
		self.y = 0

	def loadmain(self, keyboard):
		self.placement = "main"
		self.keyboard = keyboard

	def load(self, node):
		self.placement = node.getElementsByTagName('placement')[0].firstChild.data
		self.name = node.getElementsByTagName('name')[0].firstChild.data
		self.id = node.getElementsByTagName('identifiant')[0].firstChild.data
		self.keyboard = Keyboard( self.builder, node.getElementsByTagName('keyboard')[0] )
	
	def loadKeyboard(self, kbd, placement, name, id):
		self.keyboard = kbd
		self.placement = placement
		self.name = name
		self.id = id

	def getKeyboard(self):
		return self.keyboard

	def setPlacement(self, placement):
		self.placement = placement
	
	def getPlacement(self):
		return self.placement

	def getSize(self):
		(w, h) = self.keyboard.getSize()
		return (w/3.0, h/3.0)

	def setPos(self, x, y):
		self.x = x
		self.y = y

	def getXPos(self): return self.x
	def getYPos(self): return self.y

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

	def __str__(self):
		str = ""
		if self.placement != "main":
			str = str + "<extension>\n"
			str = str + "<name>" + self.name + "</name>\n"
			str = str + "<identifiant>" + self.id + "</identifiant>\n"
			str = str + "<placement>" + self.placement + "</placement>\n"
		str = str + self.keyboard.__str__()
		if self.placement != "main": str = str + "</extension>\n"
		return str

class Extensions:
	def __init__(self, builder, keyboard ):
		self.builder = builder
		self.widget = builder.get_object("exts")
		self.reset( keyboard )
		self.widget.connect("expose-event", self.expose, self)

	def reset(self, keyboard):
		self.exts = []
		self.main = Extension( self.builder )
		self.main.loadmain( keyboard )
		(w, h) = self.main.getSize()
		self.widget.set_size_request(int(w), int(h))
		self.select = self.main

	def load(self, nodes):
		self.exts = []
		for ext in nodes:
			local = Extension( self.builder )
			local.load( ext )
			self.exts.append( local )
		self.arrange()

	def arrange( self ):
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
		x = self.main.getXPos()
		y = hh
		for ext in bots:
			ext.setPos(x, y)
			(w, h) = ext.getSize()
			y += h
			hh += h

		self.widget.set_size_request(int(ww), int(hh))
		self.widget.queue_draw()
	
	def append(self, kbd):
		ext = Extension( self.builder )
		index = self.builder.get_object("placement").get_active()
		if index > 3 or index < 0:
			index = 0
			self.builder.get_object("placement").set_active(0)
			self.builder.get_object("placement").set_sensitive(True)
		placement = self.builder.get_object("placementListStore")[index][0]
		ext.loadKeyboard( kbd, placement, "noname", "noid" )
		self.exts.append( ext )
		self.select = ext
		self.arrange()
	
	def remove(self, kbd):
		if kbd == self.main: return False
		for ext in self.exts:
			if ext.getKeyboard() == kbd:
				self.exts.remove(ext)
				break
		self.arrange()
		return True

	def setPlacement(self, kbd, placement):
		if kbd == self.main: return False
		for ext in self.exts:
			if ext.getKeyboard() == kbd:
				ext.setPlacement( placement )
				break
		self.arrange()
		return True

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
		return self.select

	def getMain(self):
		return self.main.getKeyboard()

	def __str__(self):
		str = self.main.__str__()
		for ext in self.exts:
			str = str + ext.__str__()
		return str

class Editor:
	def __init__(self):
		self.changing_type = False
		self.exts = None
		self.file = None
		self.builder = gtk.Builder()
		self.builder.add_from_file("editor.glade")

		self.builder.get_object("keyboard").set_events(gtk.gdk.ALL_EVENTS_MASK)
		self.builder.get_object("exts").set_events(gtk.gdk.ALL_EVENTS_MASK)
		self.builder.get_object("editor").show()
		self.builder.get_object("extensions").show()

		self.kbd = Keyboard( self.builder )
		if len( sys.argv ) > 1:
			self.load( sys.argv[1] )
		else:
			self.new( None )
		self.kbd.connect( self.builder.get_object("keyboard") )
		self.kbd.updateSize()

		self.builder.get_object("extensions").connect("button-press-event", self.selectExtension, self)
		def onmousemove(widget, event, self):
			self.builder.get_object("hruler").emit("motion-notify-event", event);
			self.builder.get_object("vruler").emit("motion-notify-event", event);
		self.builder.get_object("keyboard").connect("motion-notify-event", onmousemove, self)
		def onkeyrelease(widget, event, self):
			if event.keyval in (gtk.keysyms.Delete, gtk.keysyms.KP_Delete):
				self.delete( widget )
		self.builder.get_object("keyboard").connect("key-release-event", onkeyrelease, self)
		self.builder.connect_signals(self)

		self.clip = gtk.Clipboard()

	def load( self, file ):
		self.file = file
		self.kbd.reset()
		xmldoc = minidom.parse(file)
		self.kbd.load(xmldoc.getElementsByTagName('keyboard')[0])
		(w, h) = self.kbd.getSize()
		self.builder.get_object("keyboard").set_size_request(int(w), int(h))
		self.updateRulers()
		if self.exts:
			self.exts.reset(self.kbd)
		else:
			self.exts = Extensions( self.builder, self.kbd )
		nodes = xmldoc.getElementsByTagName('extension')
		self.exts.load(nodes)
		self.builder.get_object("editor").set_title(os.path.basename(self.file) + " - Florence layout editor")
		self.builder.get_object("extensions").set_title(os.path.basename(self.file) + " - Florence layout extensions")
		self.kbd.updateSize()
		self.builder.get_object("placement").set_sensitive( False )

	def new( self, widget ):
		self.kbd.reset()
		if self.exts:
			self.exts.reset(self.kbd)
		else:
			self.exts = Extensions( self.builder, self.kbd )
		(w, h) = self.kbd.getSize()
		self.builder.get_object("keyboard").set_size_request(int(w), int(h))
		self.updateRulers()
		self.builder.get_object("editor").set_title("Unsaved layout - Florence layout editor")
		self.builder.get_object("extensions").set_title("Unsaved layout - Florence layout extensions")
		self.builder.get_object("placement").set_sensitive( False )


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

	def save( self, widget ):
		if not self.file:
			self.saveas( None )
		try:
			file = io.FileIO( self.file, "w" )
			file.write( self.__str__() )
		except IOError as e:
			dialog = gtk.MessageDialog( self.builder.get_object("editor"), gtk.DIALOG_DESTROY_WITH_PARENT,
				gtk.MESSAGE_ERROR, gtk.BUTTONS_OK, "Unable to save file " + self.file + " : " + e.__str__() )
			dialog.run()
			dialog.destroy()

	def saveas( self, widget ):
		chooser = gtk.FileChooserDialog( title="Save layout file as", action=gtk.FILE_CHOOSER_ACTION_SAVE,
				buttons=( gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
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
			try:
				file = io.FileIO( chooser.get_filename(), "w" )
				file.write( self.__str__() )
				self.builder.get_object("editor").set_title(os.path.basename(self.file) + \
					" - Florence layout editor")
				self.builder.get_object("exts").set_title(os.path.basename(self.file) + \
					" - Florence layout extensions")
			except IOError as e:
				dialog = gtk.MessageDialog( self.builder.get_object("editor"),
						gtk.DIALOG_DESTROY_WITH_PARENT,
						gtk.MESSAGE_ERROR, gtk.BUTTONS_OK,
							"Unable to save file " + \
							chooser.get_filename() + " : " + e.__str__() )
				dialog.run()
				dialog.destroy()
		chooser.destroy()

	def add( self, widget ):
		if self.kbd:
			x, y = self.builder.get_object("keyboard").get_pointer()
			self.kbd.add( Key("0", x / 30.0, y / 30.0, 2.0, 2.0) )
			ev = gtk.gdk.Event(gtk.gdk.BUTTON_PRESS)
			ev.x = float(x)
			ev.y = float(y)
			self.kbd.press(self.builder.get_object("keyboard"), ev, None)
	
	def delete( self, widget ):
		if self.kbd:
			self.kbd.delete()
			self.builder.get_object("editor").queue_draw()

	def cut( self, widget ):
		self.copy( widget )
		self.delete( widget )

	def copy( self, widget ):
		if self.kbd:
			self.clip.set_text("<clip>" + self.kbd.sel.__str__() + "</clip>")
			self.clip.store()

	def paste( self, widget ):
		text =  self.clip.wait_for_text()
		xmldoc = minidom.parseString( text )
		node = xmldoc.getElementsByTagName('clip')[0]
		keys = node.getElementsByTagName('key')
		self.kbd.sel.clear()
		self.kbd.clearSel()
		for k in keys:
			self.kbd.add( Key.load(k), True )
		self.builder.get_object("keyboard").queue_draw()

	def page_changed( self, widget, page_num, user_data ):
		''' Avoid infinite recursion '''
		if self.changing_type: return
		if self.builder.get_object("notebook").get_current_page() == 0:
			self.builder.get_object("actionRadio").set_active(True)
		else:
			self.builder.get_object("codeRadio").set_active(True)

	def type_changed( self, widget ):
		''' Avoid infinite recursion '''
		self.changing_type = True
		if self.builder.get_object("codeRadio").get_active():
			self.builder.get_object("notebook").set_current_page(0)
			self.code_changed( widget )
		else:
			self.builder.get_object("notebook").set_current_page(1)
			self.action_changed( widget )
		self.changing_type = False

	def code_changed( self, widget ):
		if self.kbd:
			self.kbd.setCode( self.builder.get_object("keyboard"),
				str(int(self.builder.get_object("code").get_value())) )

	def action_changed( self, widget ):
		if self.kbd:
			index = self.builder.get_object("actionComboBox").get_active()
			self.kbd.setAction( self.builder.get_object("keyboard"),
				self.builder.get_object("actions")[index][0] )

	def width_changed( self, widget ):
		if self.kbd:
			self.kbd.setWidth( self.builder.get_object("keyboard"),
				30 * self.builder.get_object("width").get_value() )

	def height_changed( self, widget ):
		if self.kbd:
			self.kbd.setHeight( self.builder.get_object("keyboard"),
				30 * self.builder.get_object("height").get_value() )

	def xpos_changed( self, widget ):
		if self.kbd:
			self.kbd.setXpos( self.builder.get_object("keyboard"),
				30 * self.builder.get_object("xpos").get_value() )

	def ypos_changed( self, widget ):
		if self.kbd:
			self.kbd.setYpos( self.builder.get_object("keyboard"),
				30 * self.builder.get_object("ypos").get_value() )

	def keyboard_width_changed( self, widget ):
		self.kbd.setSize( widget.get_value()*30, self.kbd.getHeight() )
		(w, h) = self.kbd.getSize()
		self.builder.get_object("keyboard").set_size_request(int(w), int(h))
		self.exts.arrange()

	def keyboard_height_changed( self, widget ):
		self.kbd.setSize( self.kbd.getWidth(), widget.get_value()*30 )
		(w, h) = self.kbd.getSize()
		self.builder.get_object("keyboard").set_size_request(int(w), int(h))
		self.exts.arrange()

	def placement_changed( self, widget ):
		index = self.builder.get_object("placement").get_active()
		self.exts.setPlacement( self.kbd, 
				self.builder.get_object("placementListStore")[index][0] )

	def extensions_toggled( self, widget ):
		if widget.get_active(): self.builder.get_object("extensions").show()
		else: self.builder.get_object("extensions").hide()

	def key_properties_toggled( self, widget ):
		if widget.get_active(): self.builder.get_object("properties").show()
		else: self.builder.get_object("properties").hide()

	def keyboard_properties_toggled( self, widget ):
		if widget.get_active(): self.builder.get_object("keyboard_properties").show()
		else: self.builder.get_object("keyboard_properties").hide()

	def keyboard_properties( self, widget ):
		self.builder.get_object("keyboard_properties").show()
		self.builder.get_object("keyboard_properties_menu").set_active(True)

	def key_properties( self, widget ):
		self.builder.get_object("properties").show()
		self.builder.get_object("key_properties_menu").set_active(True)

	def gg( self, widget ):
		#print self.kbd
		gtk.main_quit()

	def update_kbd(self):
		self.kbd.connect( self.builder.get_object("keyboard") )
		(w, h) = self.kbd.getSize()
		self.builder.get_object("keyboard").set_size_request(int(w), int(h))
		self.updateRulers()
		self.builder.get_object("keyboard").queue_draw()
		self.builder.get_object("extensions").queue_draw()
		self.kbd.updateSize()

	def selectExtension( self, widget, event, data ):
		self.kbd.resetSel()
		ext = self.exts.setSel(event.x, event.y)
		if ext:
			kbd = ext.getKeyboard()
			self.kbd.disconnect( self.builder.get_object("keyboard") )
			self.kbd = kbd
			self.update_kbd()
			index = 0
			for placement in self.builder.get_object("placementListStore"):
				if placement[0] == ext.getPlacement(): break
				index += 1
			self.builder.get_object("placement").set_active(index)
			self.builder.get_object("placement").set_sensitive( index < 4 )

	def add_ext( self, widget ):
		self.kbd.disconnect( self.builder.get_object("keyboard") )
		self.kbd = Keyboard( self.builder )
		self.update_kbd()
		self.exts.append( self.kbd )

	def remove_ext( self, widget ):
		self.kbd.disconnect( self.builder.get_object("keyboard") )
		self.exts.remove( self.kbd )
		self.kbd = self.exts.getMain()
		self.update_kbd()

	def updateRulers(self):
		(w, h) = self.kbd.getSize()
		self.builder.get_object("hruler").set_range(0, w/30.0, 0, w/30.0)
		self.builder.get_object("vruler").set_range(0, h/30.0, 0, h/30.0)

	def __str__(self):
		str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		str = str + "<layout xmlns:xi=\"http://www.w3.org/2001/XInclude\" xmlns=\"http://florence.sourceforge.net\">\n"
		str = str + "<informations>\n"
		str = str + "\t<name>Unnamed</name>\n"
		str = str + "\t<author>Anonymous</author>\n"
		str = str + "\t<date>%s</date>\n"%(time.strftime("%Y-%m-%d", time.gmtime(time.time())))
		str = str + "\t<florence_version>0.6.0</florence_version>\n"
		str = str + "</informations>\n"
		str = str + self.exts.__str__()
		str = str + "</layout>"
		return str

ed = Editor()
gtk.main()


