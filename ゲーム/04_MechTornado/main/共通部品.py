#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃共通処理
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
import sys; sys.path.append('..'); import 共通.MMP
from .データセット import DS

def 停止():

    共通.MMP.接続.PWM_VALUE(
        DS.仕様.ハード.装置No_屈伸  ,
        DS.仕様.ハード.モータ最小   )

    共通.MMP.接続.PWM_VALUE(
        DS.仕様.ハード.装置No_着地  ,
        DS.仕様.ハード.サーボ最大   )
