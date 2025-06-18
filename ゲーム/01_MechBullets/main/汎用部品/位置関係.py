#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃共通モジュール
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
import pyxel

class 位置関係:

	#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	#┃プレイヤーから一定距離離れた位置を得る 
	#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	def 座標取得(argDist, argX, argY):
		#┬
		#◎└┐座標が決まるまで繰り返す
		while True:
			#○ランダムに座標を求める
			x = pyxel.rndi(0, pyxel.width  - 8)
			y = pyxel.rndi(0, pyxel.height - 8)
			#│
			#○プレイヤーとの差を求める
			diff_x = x - argX
			diff_y = y - argY
			#│
			#◇┐プレイヤーとの差によって、座標を確定する
			if diff_x**2 + diff_y**2 > argDist**2:
			#　├→（指定の距離以上に離れている場合）
				#▼座標を返す
				return (x, y)
				#┴
			#　└┐（その他）
		#┴　┴　┴

	#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	#┃衝突の有無を得る
	#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
	def 衝突判定(
			argObj1,    #① 対象１((x1,y1),(x2,y,2))
			argObj2     #② 対象２((x1,y1),(x2,y,2))
			):			#【戻り値】	① 衝突あり：True／衝突なし：False
		#┬
		#○対象物１の座標範囲を求める
		obj1x1 = argObj1[0][0] + argObj1[1][0]
		obj1y1 = argObj1[0][1] + argObj1[1][1]
		obj1x2 = argObj1[0][0] + argObj1[1][2]
		obj1y2 = argObj1[0][1] + argObj1[1][3]
		#│
		#○対象物２の座標範囲を求める
		obj2x1 = argObj2[0][0] + argObj2[1][0]
		obj2y1 = argObj2[0][1] + argObj2[1][1]
		obj2x2 = argObj2[0][0] + argObj2[1][2]
		obj2y2 = argObj2[0][1] + argObj2[1][3]
		#│
		#◇┐衝突の有無を返す
		if (
			obj1x1 > obj2x2 or
			obj1x2 < obj2x1 or
			obj1y1 > obj2y2 or
			obj1y2 < obj2y1
		):
		#　├→（座標範囲が重なっていない場合）
			#▼『衝突なし』を返す
			return False
		#　└┐（その他）
		else:
			#▼『衝突あり』を返す
			return True
		#┴