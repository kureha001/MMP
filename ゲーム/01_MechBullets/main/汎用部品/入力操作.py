#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃共通モジュール（ユーザ入力操作）
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
import pyxel

#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃メイン
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 入力操作:
	
	#┬
	#□└┐移動(キー)
	ID移動_キー = (
		(pyxel.KEY_UP, pyxel.KEY_DOWN, pyxel.KEY_LEFT, pyxel.KEY_RIGHT	),
		(pyxel.KEY_8 , pyxel.KEY_2   , pyxel.KEY_4   , pyxel.KEY_6		)
	)

	#□└┐移動(パッド)
	ID移動_パッド = (
		(	pyxel.GAMEPAD1_BUTTON_DPAD_UP,
			pyxel.GAMEPAD1_BUTTON_DPAD_DOWN,
			pyxel.GAMEPAD1_BUTTON_DPAD_LEFT,
			pyxel.GAMEPAD1_BUTTON_DPAD_RIGHT
		),
		(	pyxel.GAMEPAD2_BUTTON_DPAD_UP,
			pyxel.GAMEPAD2_BUTTON_DPAD_DOWN,
			pyxel.GAMEPAD2_BUTTON_DPAD_LEFT,
			pyxel.GAMEPAD2_BUTTON_DPAD_RIGHT
		)
	)

	#□└┐ボタン(キー)
	IDボタン_キー = (
		(pyxel.KEY_Z   , pyxel.KEY_X    , pyxel.KEY_C, pyxel.KEY_SPACE	),
		(pyxel.KEY_Z   , pyxel.KEY_X    , pyxel.KEY_C, pyxel.KEY_V		),
		(pyxel.KEY_CTRL, pyxel.KEY_SHIFT, pyxel.KEY_Z, pyxel.KEY_X		)
	)

	#□└┐ボタン(パッド)
	IDボタン_パッド = (
		(	pyxel.GAMEPAD1_BUTTON_A,
			pyxel.GAMEPAD1_BUTTON_B,
			pyxel.GAMEPAD1_BUTTON_X,
			pyxel.GAMEPAD1_BUTTON_Y
		),
		(	pyxel.GAMEPAD2_BUTTON_A,
			pyxel.GAMEPAD2_BUTTON_B,
			pyxel.GAMEPAD2_BUTTON_X,
			pyxel.GAMEPAD2_BUTTON_Y
		)
	)
	#┴　┴

	#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	#┃発射有無の判定
	#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	def ワンキー():
		#┬
		#◇┐ワンキーが押されているかどうかを確認する
		if   pyxel.btn(pyxel.KEY_SPACE)				: return 1
		elif pyxel.btn(pyxel.GAMEPAD1_BUTTON_Y)		: return 1
		#　├→（押された場合）
			#○『押されている』を返す
			#┴
		#│
		#◇┐ワンキーが離されたかどうかを確認する
		if   pyxel.btnr(pyxel.KEY_SPACE)			: return -1
		elif pyxel.btnr(pyxel.GAMEPAD1_BUTTON_Y)	: return -1
		#　├→（離された場合）
			#○『離された』を返す
			#┴
		#　└┐（その他）
			#┴
		#│
		#▼『なにもなし』を返す
		return 0

	#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	#┃操作を走査
	#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	def 走査(引数_走査セット):
		#┬
		#○結果を初期化する
		概要 = [ False, False ]
		測定 = [ 0,0,0,0 ]
		#│
		#◎└┐すべての箇所を測定する
		for i in range(4):

			if	 pyxel.btn (引数_走査セット[i]):
				測定[i] = 1
				概要[0] = True

			elif pyxel.btnr(引数_走査セット[i]):
				測定[i] = -1
				概要[1] = True
			#┴
		#│
		#▼結果を返す
		return (概要,測定)