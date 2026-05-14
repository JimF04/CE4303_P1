# debug_console.gd
extends CanvasLayer

const MAX_LINEAS = 80
var _lineas: Array[String] = []
var _visible_console := false
var _label: RichTextLabel
var _ventana: PanelContainer
var _btn_toggle: Button

func _ready() -> void:
	layer = 100  # encima de todo

	# --- Botón flotante para mostrar/ocultar ---
	_btn_toggle = Button.new()
	_btn_toggle.text = "LOG"
	_btn_toggle.size = Vector2(64, 32)
	_btn_toggle.position = Vector2(10, 10)
	_btn_toggle.pressed.connect(_toggle)
	add_child(_btn_toggle)

	# --- Ventana ---
	_ventana = PanelContainer.new()
	_ventana.visible = false
	_ventana.size = Vector2(680, 300)
	_ventana.position = Vector2(10, 50)
	add_child(_ventana)

	var vbox = VBoxContainer.new()
	_ventana.add_child(vbox)

	# Barra superior
	var hbox = HBoxContainer.new()
	vbox.add_child(hbox)

	var titulo = Label.new()
	titulo.text = "  Debug Console"
	titulo.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox.add_child(titulo)

	var btn_clear = Button.new()
	btn_clear.text = "Limpiar"
	btn_clear.pressed.connect(_limpiar)
	hbox.add_child(btn_clear)

	var btn_cerrar = Button.new()
	btn_cerrar.text = "✕"
	btn_cerrar.pressed.connect(_toggle)
	hbox.add_child(btn_cerrar)

	# Área de texto con scroll
	var scroll = ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.custom_minimum_size = Vector2(0, 240)
	vbox.add_child(scroll)

	_label = RichTextLabel.new()
	_label.bbcode_enabled = true
	_label.fit_content = true
	_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_label.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.add_child(_label)

	# Interceptar print() del engine
	# Godot no tiene hook nativo, usamos un truco con el output estándar
	# → Reemplazamos las funciones de log manualmente vía señal
	pass

func log_propio(texto: String, color: String = "ffffff") -> void:
	var hora = Time.get_time_string_from_system()
	var linea = "[color=#888]%s[/color] [color=#%s]%s[/color]" % [hora, color, texto.replace("[", "（").replace("]", "）")]
	_lineas.append(linea)
	if _lineas.size() > MAX_LINEAS:
		_lineas.pop_front()
	_label.clear()
	_label.append_text("\n".join(_lineas))

func log_error(texto: String) -> void:
	log_propio(texto, "ff6b6b")

func log_ok(texto: String) -> void:
	log_propio(texto, "6bffb8")

func log_data(texto: String) -> void:
	log_propio(texto, "6bb8ff")

func _toggle() -> void:
	_visible_console = !_visible_console
	_ventana.visible = _visible_console

func _limpiar() -> void:
	_lineas.clear()
	_label.clear()
