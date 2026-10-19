# Robot Dances

Each folder here is the **complete robot program** (driving, search, chaos, eyes...).
The only difference between them is the song the dance follows. Open the `.ino` file
inside the song's folder in the Arduino IDE and upload it.

## How to dance in time

1. Put the car on the floor with some free space around it.
2. Play the song on a speaker, laptop or another phone.
3. Press dance (phone app: **4**, IR remote: **5**) exactly on the **first beat**
   (the table says how long after the song starts that is).
4. If it slowly drifts off the beat, tap dance again exactly **on** a beat.
   It lines itself up with your tap (it does not restart).
5. The screen shows the song name, the part (INTRO / VERSE / CHORUS / DROP!) and the
   time in the song, which should match the time on your music player.
6. If the moves are a bit too fast or slow for your version of the song, change
   `SONG_BPM` in section 3c of that file (a little at a time).

The buzzer plays an original tune in each song's key (bass notes on the beat plus a melody
on top), not the real melody. **Mute:** phone **X** (the "!" button) turns every sound off,
press again to turn it back on. The dance never drives forward into anything closer than 30 cm.

## The songs (39)

| Folder | Song (exact version the dance is timed to) | Tempo / key / style | Press dance | Notes |
|---|---|---|---|---|
| `Save_Me_Remy_Zero` | Save Me - Remy Zero  (4:01) | tempo 96 BPM (checked), key E major, song parts fairly sure, dance style: rock | about 1 s after the song starts (first beat) | Normal speed (the 4:01 radio edit / Smallville version). The 4:44 album version has a longer ending. |
| `National_Treasures_Drake` | National Treasures - Drake  (3:20) | tempo 129.5 BPM (fairly sure), key G# minor, song parts fairly sure, dance style: hype | about 0.9 s after the song starts (first beat) | Tempo worked out from the timed lyrics (about 130). If the moves drift, adjust SONG_BPM. |
| `The_Mentalist_Theme_Blake_Neely` | Main Title Theme (The Mentalist) [Extended Version] - Blake Neely  (1:08) | tempo 105 BPM (checked), key D minor, song parts best guess, dance style: mystery | about 0.6 s after the song starts (first beat) |  |
| `My_Destiny_Slowed_Delinquent` | My Destiny (Slowed) - Delinquent feat. KCAT  (4:39) | tempo 106 BPM (checked), key A minor, song parts fairly sure, dance style: groove | right when the song starts | The Delinquent ft. KCAT slowed release. I could NOT find this song's key: if the buzzer tune sounds off, change SONG_KEY (try 60-71) or set DANCE_SOUND to DANCE_SOUND_OFF. |
| `Confidence_Kim` | Confidence - Kim (Kim Almarcha, French zouk / R&B singer)  (3:44) | tempo 88 BPM (checked), key C# major, song parts checked, dance style: groove | about 0.7 s after the song starts (first beat) |  |
| `Distractions_Haiti_Babii` | Distractions - Haiti Babii  (2:32) | tempo 101 BPM (checked), key D major, song parts fairly sure, dance style: groove | about 0.4 s after the song starts (first beat) | Haiti Babii "Distractions". |
| `Ambarsariya_Sona_Mohapatra` | Ambarsariya - Sona Mohapatra (music Ram Sampath)  (4:08) | tempo 90 BPM (checked), key E minor, song parts fairly sure, dance style: bollywood | about 0.6 s after the song starts (first beat) | Played live, so the beat wobbles a little: tap dance on a beat at each chorus. |
| `The_Night_Begins_To_Shine_BER` | The Night Begins to Shine - B.E.R.  (3:30) | tempo 130 BPM (checked), key E minor, song parts checked, dance style: hype | about 0.5 s after the song starts (first beat) |  |
| `Love_Me_JMSN` | Love Me - JMSN  (4:33) | tempo 81 BPM (checked), key A# minor, song parts best guess, dance style: ballad | about 0.3 s after the song starts (first beat) | JMSN "Love Me". No timed lyrics exist for it, so the song parts are an estimate. |
| `Loverboy_A_Wall` | Loverboy - A-Wall  (3:45) | tempo 120 BPM (checked), key C major, song parts fairly sure, dance style: groove | about 0.4 s after the song starts (first beat) |  |
| `The_Kill_2_Lex_Amarni` | the kill 2 - Lex Amarni & 2muchmotion  (2:01) | tempo 103 BPM (checked), key D minor, song parts fairly sure, dance style: hype | about 0.1 s after the song starts (first beat) |  |
| `Gods_Plan_Drake` | God's Plan - Drake  (3:19) | tempo 77 BPM (checked), key E minor, song parts fairly sure, dance style: groove | right when the song starts |  |
| `Oh_Yeah_Steve_Lacy` | oh yeah? - Steve Lacy  (2:51) | tempo 73 BPM (fairly sure), key C# major, song parts best guess, dance style: groove | about 0.3 s after the song starts (first beat) |  |
| `Loser_Tame_Impala` | Loser - Tame Impala  (3:44) | tempo 83 BPM (checked), key A# minor, song parts fairly sure, dance style: groove | about 0.9 s after the song starts (first beat) |  |
| `Aal_Izz_Well_3_Idiots` | Aal Izz Well - Sonu Nigam, Shaan & Swanand Kirkire (3 Idiots, music Shantanu Moitra)  (4:36) | tempo 120 BPM (fairly sure), key D# major, song parts fairly sure, dance style: bollywood | about 1.8 s after the song starts (first beat) | Calm parts at normal speed; the party parts (choruses) are POWER parts: explosive moves at extra speed. The dance is silent (DANCE_SOUND_OFF) - set DANCE_SOUND_TUNE for the tune. |
| `Chammak_Challo_Ra_One` | Chammak Challo - Akon & Hamsika Iyer (Ra.One, music Vishal-Shekhar)  (3:47) | tempo 134.3 BPM (fairly sure), key B minor, song parts fairly sure, dance style: explosive | about 0.3 s after the song starts (first beat) | Tempo worked out from the timed lyrics (2 bars per line = 134 BPM); key from a chord site. If it drifts, tap dance on a beat. Phone X mutes the buzzer. |
| `Feet_Dont_Fail_Me_Now_Joy_Crookes` | Feet Don't Fail Me Now - Joy Crookes  (3:18) | tempo 113 BPM (checked), key A# minor, song parts fairly sure, dance style: hype | right when the song starts |  |
| `Kabhi_Kabhi_Aditi_Rashid_Ali` | Kabhi Kabhi Aditi - Rashid Ali (music: A.R. Rahman)  (3:38) | tempo 95 BPM (checked), key G minor, song parts fairly sure, dance style: bollywood | about 2.2 s after the song starts (first beat) |  |
| `Bolve_Madan_Maddi` | Bolve - Madan Maddi & Sukshinder Shinda  (5:12) | tempo 114 BPM (checked), key E minor, song parts fairly sure, dance style: bollywood | about 0.3 s after the song starts (first beat) | The big bhangra parts are POWER parts: explosive moves at extra speed. |
| `Flashing_Lights_Kanye_West` | Flashing Lights (feat. Dwele) - Kanye West  (3:57) | tempo 90.5 BPM (checked), key F# minor, song parts fairly sure, dance style: groove | right when the song starts |  |
| `Touch_The_Sky_Kanye_West` | Touch The Sky - Kanye West feat. Lupe Fiasco  (3:57) | tempo 106.7 BPM (checked), key B minor, song parts checked, dance style: hype | right when the song starts |  |
| `Aankhen_Milaney_Waaley_Nazia_Hassan` | Aankhen Milaney Waaley - Nazia Hassan & Zoheb Hassan  (3:34) | tempo 107.4 BPM (checked), key F# major, song parts fairly sure, dance style: hype | about 0.3 s after the song starts (first beat) |  |
| `We_Are_The_People_Empire_Of_The_Sun` | We Are The People - Empire Of The Sun  (4:32) | tempo 123.4 BPM (checked), key E minor, song parts checked, dance style: hype | about 0.9 s after the song starts (first beat) |  |
| `Get_Lucky_Drumless_Daft_Punk` | Get Lucky (Drumless Edition) - Daft Punk feat. Pharrell Williams & Nile Rodgers  (6:09) | tempo 116 BPM (checked), key F# minor, song parts fairly sure, dance style: groove | right when the song starts |  |
| `Greenback_Boogie_Suits_Theme` | Greenback Boogie (Suits Main Title Edit) - Ima Robot  (0:31) | tempo 143 BPM (checked), key F major, song parts fairly sure, dance style: hype | about 1 s after the song starts (first beat) |  |
| `Copacabana_Barry_Manilow` | Copacabana (At the Copa) - Barry Manilow  (4:05) | tempo 116.6 BPM (checked), key D# major, song parts fairly sure, dance style: hype | about 0.1 s after the song starts (first beat) |  |
| `They_Dont_Care_About_Us_Michael_Jackson` | They Don't Care About Us - Michael Jackson  (4:44) | tempo 90 BPM (checked), key D minor, song parts fairly sure, dance style: rock | about 1.9 s after the song starts (first beat) |  |
| `Upside_Down_Diana_Ross` | Upside Down - Diana Ross  (4:05) | tempo 108.5 BPM (checked), key G minor, song parts checked, dance style: groove | about 1.4 s after the song starts (first beat) |  |
| `My_Type_Saint_Motel` | My Type - Saint Motel  (3:26) | tempo 118 BPM (checked), key C minor, song parts fairly sure, dance style: hype | about 0.7 s after the song starts (first beat) |  |
| `Baba_Bandook_Ali_Azmat` | Baba Bandook - Ali Azmat & Haroon  (3:53) | tempo 88.5 BPM (fairly sure), key D major, song parts best guess, dance style: hype | about 0.3 s after the song starts (first beat) |  |
| `Iraaday_Rovalio` | Iraaday - Rovalio & Abdul Hannan  (2:13) | tempo 90 BPM (checked), key F# major, song parts fairly sure, dance style: groove | right when the song starts |  |
| `Worry_LONOWN` | worry - LONOWN (feat. riserayss)  (3:01) | tempo 99 BPM (fairly sure), key B major, song parts fairly sure, dance style: hype | about 0.5 s after the song starts (first beat) |  |
| `Sh_Boom_The_Chords_Cars` | Sh-Boom (Life Could Be a Dream) - The Chords  (2:26) | tempo 68.5 BPM (checked), key F major, song parts checked, dance style: cruise | about 0.8 s after the song starts (first beat) | Slow, jazzy cruising like the Cars neon-lights scene: one move per beat (the tempo here is half the song tempo on purpose). |
| `Ben_10_1_Classic_Theme` | Ben 10 Theme Song (original series opening) - Moxy (written by Andy Sturmer)  (1:01) | tempo 136 BPM (checked), key D# minor, song parts fairly sure, dance style: rock | about 0.1 s after the song starts (first beat) | The 1:01 TV opening. Works with the English AND the Hindi/Urdu version (same music and length). The title hit flashes BEN 10! on the screen. |
| `Ben_10_2_Alien_Force_Theme` | Ben 10: Alien Force Theme Song (Main Title / Opening) - Andy Sturmer (Cartoon Network)  (0:31) | tempo 135 BPM (checked), key C minor, song parts best guess, dance style: hype | about 0.2 s after the song starts (first beat) | The 0:31 TV opening. Works with the English AND the Hindi/Urdu version (same music and length). |
| `Ben_10_3_Ultimate_Alien_Theme` | Ben 10: Ultimate Alien Theme Song (Opening Theme) - Kristopher Carter, Michael McCuistion & Lolita Ritmanis (Cartoon Network)  (0:21) | tempo 85 BPM (fairly sure), key C minor, song parts best guess, dance style: hype | about 0.2 s after the song starts (first beat) | The 0:21 TV opening. Works with the English AND the Hindi/Urdu version. The song is very fast (170), so the robot dances one move per beat. |
| `Ben_10_4_Omniverse_Theme` | Ben 10: Omniverse Theme Song (main opening) - Parry Gripp (writer and performer)  (0:20) | tempo 98 BPM (fairly sure), key B minor, song parts best guess, dance style: hype | about 0.3 s after the song starts (first beat) | The 0:21 TV opening. Works with the English AND the Hindi/Urdu version. FEEDBACK! flashes (with an electric zap) where Feedback appears. The song is very fast (196), so the robot dances one move per beat. |
| `Ben_10_5_Omniverse_Kid_Ben` | Ben 10: Omniverse Theme Song - Opening #1 (the version built around 11-year-old Ben, shown split-screen with 16-year-old Ben) - Parry Gripp (writer and performer)  (0:21) | tempo 98 BPM (fairly sure), key B minor, song parts best guess, dance style: groove | about 0.2 s after the song starts (first beat) | Opening #1 (Season 1 to S2E7 Bros in Space) - the one built around 11-year-old Ben, same song as the later Omniverse opening. FEEDBACK! flashes three times where kid-Ben Feedback appears. |
| `Badtameez_Dil_Pritam` | Badtameez Dil - Benny Dayal & Shefali Alvares (Yeh Jawaani Hai Deewani, music Pritam)  (4:13) | tempo 106 BPM (checked), key D minor, song parts fairly sure, dance style: bollywood | right when the song starts (first beat at about 0.15 s) | The 4:13 album version. The choruses and the brass dance break are POWER parts: explosive moves at extra speed. |
