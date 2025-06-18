#┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#┃データセット：情報
#┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class 情報クラス:
    def __init__(self):
        self.フレーム数 = 0
        self.シーン     = None
        self.ボスシーン = None
        self.プレイ時間 = 0
        self.難易度     = 0
        self.得点       = 0
        self.人数       = 0
        self.音楽データ = None
        self.再生時間   = None
        self.操作手段   = None
        self.MMP中央値  = 450
        self.MMP反応率  = 10