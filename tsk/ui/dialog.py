# tsk/ui/dialog.py
import pyray as rl

from openpilot.system.ui.lib.application import gui_app
from openpilot.system.ui.lib.scroll_panel import GuiScrollPanel
from openpilot.system.ui.widgets.button import gui_button, DEFAULT_BUTTON_FONT_SIZE, ButtonStyle


class BaseDialog:
  """Base class for full-screen dialogs with a scrollable text area."""

  BORDER_SIZE = 20
  BUTTON_HEIGHT = 80
  BUTTON_WIDTH = 310
  BUTTON_SPACING = 20
  FONT_SIZE = 100
  LINE_HEIGHT = FONT_SIZE * 1.1
  TEXT_PADDING = 10

  def __init__(self, body_text: str, font_size: int = FONT_SIZE, scroll_to_bottom: bool = False):
    self.body_text = body_text
    self.font_size = font_size
    self.scroll_to_bottom = scroll_to_bottom
    self.LINE_HEIGHT = self.font_size * 1.1
    self.textarea_rect = rl.Rectangle(
      self.BORDER_SIZE,
      self.BORDER_SIZE,
      gui_app.width - 2 * self.BORDER_SIZE,
      gui_app.height - 3 * self.BORDER_SIZE - self.BUTTON_HEIGHT  # Account for buttons and spacing
    )
    self.wrapped_lines = self._wrap_text(self.body_text, self.font_size, self.textarea_rect.width - 2 * self.TEXT_PADDING)
    self.content_height = len(self.wrapped_lines) * self.LINE_HEIGHT
    self.content_rect = rl.Rectangle(0, 0, self.textarea_rect.width - 2 * self.TEXT_PADDING, self.content_height)
    self.scroll_panel = GuiScrollPanel(show_vertical_scroll_bar=True)
    self.scroll_offset = rl.Vector2(0, 0) # Store the scroll offset
    self.initial_scroll_applied = False  # Flag to track initial scroll application

  def render_text_area(self):
    """Renders the scrollable text area."""

    scroll = self.scroll_panel.handle_scroll(self.textarea_rect, self.content_rect)
    self.scroll_offset = scroll # Update the scroll offset after handling user input

    # Apply initial scroll to bottom after the first render
    if self.scroll_to_bottom and not self.initial_scroll_applied:
      self.scroll_offset.y = min(0, self.textarea_rect.height - self.content_height - 2 * self.TEXT_PADDING)
      self.initial_scroll_applied = True

    rl.begin_scissor_mode(int(self.textarea_rect.x), int(self.textarea_rect.y), int(self.textarea_rect.width), int(self.textarea_rect.height))
    y_offset = 0
    for line in self.wrapped_lines:
      position = rl.Vector2(self.textarea_rect.x + self.TEXT_PADDING + self.scroll_offset.x, self.textarea_rect.y + self.TEXT_PADDING + self.scroll_offset.y + y_offset)
      if position.y + self.LINE_HEIGHT < self.textarea_rect.y + self.TEXT_PADDING or position.y > self.textarea_rect.y + self.textarea_rect.height - self.TEXT_PADDING:
        y_offset += self.LINE_HEIGHT
        continue
      rl.draw_text_ex(gui_app.font(), line.strip(), position, self.font_size, 0, rl.WHITE)
      y_offset += self.LINE_HEIGHT
    rl.end_scissor_mode()

  def _wrap_text(self, text, font_size, max_width):
    lines = []
    font = gui_app.font()

    # Split the text by newline characters
    for block in text.splitlines():
      if not block:  # Handle empty lines (consecutive newlines)
        lines.append("")  # Add an empty line
        continue

      current_line = ""
      for word in block.split():
        test_line = current_line + word + " "
        if rl.measure_text_ex(font, test_line, font_size, 0).x <= max_width:
          current_line = test_line
        else:
          lines.append(current_line)
          current_line = word + " "
      if current_line:
        lines.append(current_line)

    return lines


class OkayDialog(BaseDialog):
  """A full-screen dialog with a scrollable text area and an Okay button."""

  @staticmethod
  def ask(body_text: str, font_size: int = BaseDialog.FONT_SIZE, scroll_to_bottom: bool = False, okay_text: str = "Okay") -> None:
    """Displays a full-screen Okay dialog."""
    okay_pressed = False

    dialog = OkayDialog(body_text, font_size, scroll_to_bottom)

    def render_dialog():
      nonlocal okay_pressed  # Allow modification of the okay_pressed variable

      dialog.render_text_area()

      # Calculate the available height for the button area
      button_area_height = gui_app.height - dialog.textarea_rect.height - dialog.textarea_rect.y

      # Button position (centered horizontally, vertically centered in the button area)
      button_x = (gui_app.width - BaseDialog.BUTTON_WIDTH) / 2
      button_y = dialog.textarea_rect.y + dialog.textarea_rect.height + (button_area_height - BaseDialog.BUTTON_HEIGHT) / 2

      # TOUCH RELIABILITY FIX: Use fixed gui_button instead of custom button
      # The original code used gui_button which now has reliable touch handling
      if gui_button(rl.Rectangle(button_x, button_y, BaseDialog.BUTTON_WIDTH, BaseDialog.BUTTON_HEIGHT), okay_text):
        okay_pressed = True

    # Main loop
    while not okay_pressed and not rl.window_should_close():
      rl.begin_drawing()
      rl.clear_background(rl.BLACK)

      render_dialog()

      rl.end_drawing()


class YesNoDialog(BaseDialog):
  """A full-screen dialog with a scrollable text area and Yes/No buttons."""

  @staticmethod
  def ask(body_text: str, font_size: int = BaseDialog.FONT_SIZE, scroll_to_bottom: bool = False, yes_text: str = "Yes", no_text: str = "No") -> bool | None:
    """Displays a full-screen Yes/No dialog and returns a boolean based on the user's choice."""
    result = None  # Use None to indicate the dialog is still active

    dialog = YesNoDialog(body_text, font_size, scroll_to_bottom)

    def render_dialog():
      nonlocal result  # Allow modification of the result variable

      dialog.render_text_area()

      button_top = gui_app.height - BaseDialog.BORDER_SIZE - BaseDialog.BUTTON_HEIGHT
      no_button_x = BaseDialog.BORDER_SIZE
      yes_button_x = gui_app.width - BaseDialog.BORDER_SIZE - BaseDialog.BUTTON_WIDTH

      no_button_rect = rl.Rectangle(no_button_x, button_top, BaseDialog.BUTTON_WIDTH, BaseDialog.BUTTON_HEIGHT)
      yes_button_rect = rl.Rectangle(yes_button_x, button_top, BaseDialog.BUTTON_WIDTH, BaseDialog.BUTTON_HEIGHT)

      # TOUCH RELIABILITY FIX: Replace draw_custom_button with hybrid approach
      #
      # ORIGINAL PROBLEM: draw_custom_button only handled mouse input
      # The custom button function only checked rl.is_mouse_button_released()
      # and completely ignored touch input, causing unreliable behavior on touch devices.
      #
      # SOLUTION: Use hybrid approach - gui_button for touch detection + custom rendering for colors
      # - gui_button() provides reliable touch handling (invisible, NO_EFFECT style)
      # - Custom drawing code maintains the original button colors and appearance
      # - This gives us both reliable input handling AND the original visual design

      # No button - dark red color as originally designed
      no_clicked = gui_button(no_button_rect, "", font_size=1, button_style=ButtonStyle.NO_EFFECT)
      # Draw original No button appearance with dark red color
      rl.draw_rectangle_rec(no_button_rect, rl.Color(100, 20, 20, 255))  # Original dark red
      font = gui_app.font()
      text_size = rl.measure_text_ex(font, no_text, DEFAULT_BUTTON_FONT_SIZE, 0)
      text_x = no_button_rect.x + (no_button_rect.width - text_size.x) / 2
      text_y = no_button_rect.y + (no_button_rect.height - text_size.y) / 2
      rl.draw_text_ex(font, no_text, rl.Vector2(text_x, text_y), DEFAULT_BUTTON_FONT_SIZE, 0, rl.WHITE)

      if no_clicked:
        result = False

      # Yes button - dark green color as originally designed
      yes_clicked = gui_button(yes_button_rect, "", font_size=1, button_style=ButtonStyle.NO_EFFECT)
      # Draw original Yes button appearance with dark green color
      rl.draw_rectangle_rec(yes_button_rect, rl.Color(20, 100, 20, 255))  # Original dark green
      text_size = rl.measure_text_ex(font, yes_text, DEFAULT_BUTTON_FONT_SIZE, 0)
      text_x = yes_button_rect.x + (yes_button_rect.width - text_size.x) / 2
      text_y = yes_button_rect.y + (yes_button_rect.height - text_size.y) / 2
      rl.draw_text_ex(font, yes_text, rl.Vector2(text_x, text_y), DEFAULT_BUTTON_FONT_SIZE, 0, rl.WHITE)

      if yes_clicked:
        result = True

    # Main loop
    while result is None and not rl.window_should_close():
      rl.begin_drawing()
      rl.clear_background(rl.BLACK)

      render_dialog()

      rl.end_drawing()

    return result


# TOUCH RELIABILITY FIX: Removed draw_custom_button function
#
# ORIGINAL PROBLEM: This function only handled mouse input
# The draw_custom_button function had the same core issue as the original gui_button:
# - Only checked mouse events: rl.is_mouse_button_released(rl.MouseButton.MOUSE_BUTTON_LEFT)
# - No touch input handling whatsoever
# - Caused dialog buttons to require multiple taps on touch devices
#
# SOLUTION: Replaced with hybrid approach using fixed gui_button
# Instead of fixing this custom function, we replaced it with the reliable gui_button
# for input handling while preserving the original visual appearance through custom drawing.
# This eliminates code duplication and ensures consistent touch behavior across all buttons.
