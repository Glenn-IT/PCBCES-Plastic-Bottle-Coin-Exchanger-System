# PCBCES Bottle Mode & Pricing Specifications (Updated & Confirmed)

## 1. 1.5L / 1.75L Bottle Mode
- **Mode Button:** Green Button (Arduino Pin D10)
- **Bottle Classification:** 1.5 Liter / 1.75 Liter PET Bottles (Height: ~30–33 cm; Distance to Cap: 7–15 cm)
- **Session Quota:** Exactly 5 bottles per session (LCD displays `0/5` up to `5/5`)
- **Reward Rate:** ₱4.00 per bottle
- **Total Payout:** 5 bottles × ₱4.00 = **₱20.00 (20 x ₱1 coins)** dispensed via 12V Coin Hopper

## 2. 290 ML Bottle Mode (Formerly "Mismo")
- **Mode Button:** Blue Button (Arduino Pin A0)
- **Naming Standard:** Renamed strictly from "Mismo" to **"290 ML"**
- **Bottle Classification:** 290 ML PET Bottles (Height: ~16–17 cm; Distance to Cap: 26–27 cm)
- **Session Quota:** Exactly 10 bottles per session (LCD displays `0/10` up to `10/10`)
- **Total Payout:** 10 bottles = **₱3.00 (3 x ₱1 coins)** dispensed via 12V Coin Hopper

## 3. Control & Safety
- **Red Button (Arduino Pin A1):** Transaction Cancel / System Reset at any time (clears count, returns to standby menu)
