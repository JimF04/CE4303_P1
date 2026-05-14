# hud_overlay.gd
extends CanvasLayer

signal salir_solicitado  # ← Map escucha esto para correr la animación

const MAX_LINEAS = 80
var _lineas: Array[String] = []
var _visible_panel := false
var _visible_debug := false
var _label: RichTextLabel
var _panel_settings: PanelContainer
var _panel_debug: PanelContainer

func _ready() -> void:
	layer = 100

	# --- Botón ⚙ ---
	var btn_settings := Button.new()
	btn_settings.text = "⚙"
	btn_settings.size = Vector2(48, 48)
	btn_settings.position = Vector2(10, 10)
	btn_settings.pressed.connect(_toggle_settings)
	add_child(btn_settings)

	# ── Panel Settings ──────────────────────────────────────────────────────
	_panel_settings = PanelContainer.new()
	_panel_settings.visible = false
	_panel_settings.size = Vector2(300, 160)
	_panel_settings.position = Vector2(10, 66)
	add_child(_panel_settings)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 12)
	_panel_settings.add_child(vbox)

	# Título + cerrar
	var hbox_top := HBoxContainer.new()
	vbox.add_child(hbox_top)

	var titulo := Label.new()
	titulo.text = "  Ajustes"
	titulo.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox_top.add_child(titulo)

	var btn_x := Button.new()
	btn_x.text = "✕"
	btn_x.pressed.connect(_toggle_settings)
	hbox_top.add_child(btn_x)

	vbox.add_child(HSeparator.new())

	# Botón Debug Console
	var btn_debug := Button.new()
	btn_debug.text = "🖥  Debug Console"
	btn_debug.custom_minimum_size = Vector2(0, 44)
	btn_debug.pressed.connect(_toggle_debug)
	vbox.add_child(btn_debug)

	# Botón Cerrar aplicación
	var btn_salir := Button.new()
	btn_salir.text = "🚪  Cerrar aplicación"
	btn_salir.custom_minimum_size = Vector2(0, 44)
	btn_salir.pressed.connect(_pedir_salir)
	vbox.add_child(btn_salir)

	# ── Panel Debug ─────────────────────────────────────────────────────────
	_panel_debug = PanelContainer.new()
	_panel_debug.visible = false
	_panel_debug.size = Vector2(680, 320)
	_panel_debug.position = Vector2(10, 66)
	add_child(_panel_debug)

	var vbox_d := VBoxContainer.new()
	_panel_debug.add_child(vbox_d)

	var hbox_d := HBoxContainer.new()
	vbox_d.add_child(hbox_d)

	var tit_d := Label.new()
	tit_d.text = "  Debug Console"
	tit_d.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox_d.add_child(tit_d)

	var btn_clear := Button.new()
	btn_clear.text = "Limpiar"
	btn_clear.pressed.connect(_limpiar)
	hbox_d.add_child(btn_clear)

	var btn_xd := Button.new()
	btn_xd.text = "✕"
	btn_xd.pressed.connect(_toggle_debug)
	hbox_d.add_child(btn_xd)

	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.custom_minimum_size = Vector2(0, 260)
	vbox_d.add_child(scroll)

	_label = RichTextLabel.new()
	_label.bbcode_enabled = true
	_label.fit_content = true
	_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_label.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.add_child(_label)

func _toggle_settings() -> void:
	_visible_panel = !_visible_panel
	_panel_settings.visible = _visible_panel
	# cerrar debug si se cierra settings
	if not _visible_panel:
		_visible_debug = false
		_panel_debug.visible = false

func _toggle_debug() -> void:
	_visible_debug = !_visible_debug
	_panel_debug.visible = _visible_debug

func _pedir_salir() -> void:
	_panel_settings.visible = false
	_panel_debug.visible = false
	salir_solicitado.emit()  # <- Map recibe esto y corre _iniciar_salida()

func _limpiar() -> void:
	_lineas.clear()
	_label.clear()

func log_propio(texto: String, color: String = "ffffff") -> void:
	var hora := Time.get_time_string_from_system()
	var linea := "[color=#888]%s[/color] [color=#%s]%s[/color]" % [hora, color, texto.replace("[", "（").replace("]", "）")]
	_lineas.append(linea)
	if _lineas.size() > MAX_LINEAS:
		_lineas.pop_front()
	_label.clear()
	_label.append_text("\n".join(_lineas))

func log_error(texto: String) -> void: log_propio(texto, "ff6b6b")
func log_ok(texto: String)    -> void: log_propio(texto, "6bffb8")
func log_data(texto: String)  -> void: log_propio(texto, "6bb8ff")
