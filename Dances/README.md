# Robot Dances

Each folder here is the **complete robot program** (driving, search, chaos, eyes...).
The only difference between them is the song and its **choreography**. Open the `.ino` file
inside the song's folder in the Arduino IDE and upload it.

## How the choreography works

Every song has its own dance steps (its signature moves) and a routine for each part of the song.
A routine starts again every time that part comes back, so **every chorus has the same chorus
routine**, like a real dance: you will recognise it. Phrases end on a pose, the bar before a big
part is a lead-in move, and the last bar is a finale. Spins marked *round* go exactly once round
and stop facing the front (timed like the 180 turn), and every step brings the car back to its
spot, so it dances in place. The routines are written out in plain words at the top of each file
(THE CHOREOGRAPHY) and the steps are in `SONG_MOVES`, so you can change them.

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

| Folder | Song (exact version the dance is timed to) | The dance | Tempo / key | Press dance | Notes |
|---|---|---|---|---|---|
| `Save_Me_Remy_Zero` | Save Me - Remy Zero  (4:01) | A Superman anthem: calm, strong poses in the verses, flying swoops in the choruses and a big take-off in the last chorus. | 96 BPM (checked), E major | about 1.04 s after the song starts | Normal speed (the 4:01 radio edit / Smallville version). The 4:44 album version has a longer ending. |
| `National_Treasures_Drake` | National Treasures - Drake  (3:20) | Cold trap swagger: slow nods and icy glides while Drake raps, hard freeze-hits on the hook after the beat switch. | 129.5 BPM (fairly sure), G# minor | about 0.9 s after the song starts | Tempo worked out from the timed lyrics (about 130). If the moves drift, adjust SONG_BPM. |
| `The_Mentalist_Theme_Blake_Neely` | Main Title Theme (The Mentalist) [Extended Version] - Blake Neely  (1:08) | A detective on the case: creeps forward, freezes, looks around, and spins round suddenly when it smells a clue. | 105 BPM (checked), D minor | about 0.59 s after the song starts |  |
| `My_Destiny_Slowed_Delinquent` | My Destiny (Slowed) - Delinquent feat. KCAT  (4:39) | Dreamy UK garage: floaty slow-motion moves, with the garage skip-step in the choruses. | 106 BPM (checked), A minor | right when the song starts | The Delinquent ft. KCAT slowed release. I could NOT find this song's key: if the buzzer tune sounds off, change SONG_KEY (try 60-71) or set DANCE_SOUND to DANCE_SOUND_OFF. |
| `Confidence_Kim` | Confidence - Kim (Kim Almarcha, French zouk / R&B singer)  (3:44) | Smooth zouk: hip sways and glides (the moves you liked), arranged into a routine - the same chorus routine every chorus, call-and-response in the bridge. | 88 BPM (checked), C# major | about 0.7 s after the song starts |  |
| `Distractions_Haiti_Babii` | Distractions - Haiti Babii  (2:32) | Moody anime-edit vibe: sad slow sways and head nods in the verses, sharp edit-style freezes on the hook. | 101 BPM (checked), D major | about 0.4 s after the song starts | Haiti Babii "Distractions". |
| `Ambarsariya_Sona_Mohapatra` | Ambarsariya - Sona Mohapatra (music Ram Sampath)  (4:08) | Punjabi folk charm: jhoom sways and thumka pops in the verses, bhangra in the choruses (the moves you liked, now a real routine). | 90 BPM (checked), E minor | about 0.63 s after the song starts | Played live, so the beat wobbles a little: tap dance on a beat at each chorus. |
| `The_Night_Begins_To_Shine_BER` | The Night Begins to Shine - B.E.R.  (3:30) | 80s synthwave aerobics: neon hustle steps and disco points, big jumps and spins in the choruses, and the longest party in the final chorus. | 130 BPM (checked), E minor | about 0.5 s after the song starts |  |
| `Love_Me_JMSN` | Love Me - JMSN  (4:33) | Slow, cool R&B: minimal slow-motion moves and smooth glides, a cool spin-and-pose at the end of every chorus line. | 81 BPM (checked), A# minor | about 0.3 s after the song starts | JMSN "Love Me". No timed lyrics exist for it, so the song parts are an estimate. |
| `Loverboy_A_Wall` | Loverboy - A-Wall  (3:45) | Dreamy funk: easy side-to-side sways and nods, a hustle-and-spin routine in every chorus and a looser groove after it. | 120 BPM (checked), C major | about 0.4 s after the song starts |  |
| `The_Kill_2_Lex_Amarni` | the kill 2 - Lex Amarni & 2muchmotion  (2:01) | Bouncy UK cloud rap: bounce on every beat, sharp hits on the hook and a shimmy answer. | 103 BPM (checked), D minor | about 0.1 s after the song starts |  |
| `Gods_Plan_Drake` | God's Plan - Drake  (3:19) | Laid-back Drake bounce: head nods and cool steps in the verses, a smooth bounce and slow-motion spin on the hook. | 77 BPM (checked), E minor | right when the song starts |  |
| `Oh_Yeah_Steve_Lacy` | oh yeah? - Steve Lacy  (2:51) | Chill neo-soul: relaxed sways and head nods, a little two-step in the choruses. | 73 BPM (fairly sure), C# major | about 0.3 s after the song starts |  |
| `Loser_Tame_Impala` | Loser - Tame Impala  (3:44) | Funky psych-pop trot: trotting steps all through, a bigger trot-and-spin routine in the choruses. | 83 BPM (checked), A# minor | about 0.9 s after the song starts |  |
| `Aal_Izz_Well_3_Idiots` | Aal Izz Well - Sonu Nigam, Shaan & Swanand Kirkire (3 Idiots, music Shantanu Moitra)  (4:36) | Aal Izz Well: the chest-pat sway in the verses ("all is well" - calm down), and when the party kicks in, the famous bouncy chorus with whistle looks and big spins. | 120 BPM (fairly sure), D# major | about 1.8 s after the song starts | Calm parts at normal speed; the party parts (choruses) are POWER parts: explosive moves at extra speed. The dance is silent (DANCE_SOUND_OFF) - set DANCE_SOUND_TUNE for the tune. |
| `Chammak_Challo_Ra_One` | Chammak Challo - Akon & Hamsika Iyer (Ra.One, music Vishal-Shekhar)  (3:47) | Chammak Challo: the shoulder-pop hook step and thumka on every chorus, smooth R&B sways in the verses, big dhol spins at the end. | 134.3 BPM (fairly sure), B minor | about 0.35 s after the song starts | Tempo worked out from the timed lyrics (2 bars per line = 134 BPM); key from a chord site. If it drifts, tap dance on a beat. Phone X mutes the buzzer. |
| `Feet_Dont_Fail_Me_Now_Joy_Crookes` | Feet Don't Fail Me Now - Joy Crookes  (3:18) | 70s funk strut: struts forward and back in the verses, fast "feet" footwork and stomps in the choruses. | 113 BPM (checked), A# minor | right when the song starts |  |
| `Kabhi_Kabhi_Aditi_Rashid_Ali` | Kabhi Kabhi Aditi - Rashid Ali (music: A.R. Rahman)  (3:38) | Carefree college-friends feel: a happy bouncy walk in the verses, sing-along sways in the chorus, and the final tag repeats getting bigger each time. | 95 BPM (checked), G minor | about 2.2 s after the song starts |  |
| `Bolve_Madan_Maddi` | Bolve - Madan Maddi & Sukshinder Shinda  (5:12) | Proper bhangra: dhol bounces, shoulder pops and the bhangra step, with the explosive POWER bhangra in the hooks and dhol breaks. | 114 BPM (checked), E minor | about 0.3 s after the song starts | The big bhangra parts are POWER parts: explosive moves at extra speed. |
| `Badtameez_Dil_Pritam` | Badtameez Dil - Benny Dayal & Shefali Alvares (music Pritam)  (4:13) | Badtameez Dil: three hits for "badtameez dil, badtameez dil, badtameez dil", a head-shaking shimmy for "maane na", and wild spins in the brass dance break. | 106 BPM (checked), D minor | about 0.15 s after the song starts | Tempo 106 BPM (songbpm; the timed lyrics agree). The choruses and the brass dance break are POWER parts: explosive moves at extra speed. |
| `Flashing_Lights_Kanye_West` | Flashing Lights (feat. Dwele) - Kanye West  (3:57) | Night drive: cool slow-motion glides and swagger, with quick "flashing lights" head flashes left and right on the hook and chorus. | 90.5 BPM (checked), F# minor | right when the song starts |  |
| `Touch_The_Sky_Kanye_West` | Touch The Sky - Kanye West feat. Lupe Fiasco  (3:57) | Triumphant horns: a hit on every horn blast, "reach for the sky" take-offs in the choruses and jumping on the "sky high" chant. | 106.7 BPM (checked), B minor | right when the song starts |  |
| `Aankhen_Milaney_Waaley_Nazia_Hassan` | Aankhen Milaney Waaley - Nazia Hassan & Zoheb Hassan  (3:34) | 80s disco-pop: disco points and the hustle, a spin-and-point chorus, playful looks for "aankhen milaney" (eyes meeting). | 107.4 BPM (checked), F# major | about 0.3 s after the song starts |  |
| `We_Are_The_People_Empire_Of_The_Sun` | We Are The People - Empire Of The Sun  (4:32) | Sunny, dreamy electro-pop: floating slow-motion moves, waves and figure-eights, and euphoric spins in the big sing-along choruses. | 123.4 BPM (checked), E minor | about 0.93 s after the song starts |  |
| `Get_Lucky_Drumless_Daft_Punk` | Get Lucky (Drumless Edition) - Daft Punk feat. Pharrell Williams & Nile Rodgers  (6:09) | Disco-funk: the hustle and disco points to the guitar, and sharp ROBOT moves (freeze-and-snap) when the Daft Punk robot voices come in. | 116 BPM (checked), F# minor | right when the song starts |  |
| `Greenback_Boogie_Suits_Theme` | Greenback Boogie (Suits Main Title Edit) - Ima Robot  (0:31) | The Suits intro: a cool, confident strut with look-overs, a sharp call-to-dance shimmy, and a sharp final pose. | 143 BPM (checked), F major | about 1 s after the song starts |  |
| `Copacabana_Barry_Manilow` | Copacabana (At the Copa) - Barry Manilow  (4:05) | At the Copa: mambo breaks and cha-cha-chas, salsa spins in the choruses, and a full samba party in the dance break. | 116.6 BPM (checked), D# major | about 0.12 s after the song starts |  |
| `They_Dont_Care_About_Us_Michael_Jackson` | They Don't Care About Us - Michael Jackson  (4:44) | MJ: military stomps and freezes, head snaps left and right, MJ spins into a pose, and a stomp-and-pump climax for the guitar solo. | 90 BPM (checked), D minor | about 1.9 s after the song starts |  |
| `Upside_Down_Diana_Ross` | Upside Down - Diana Ross  (4:05) | Disco-funk: the hustle and bouncy steps, and for "upside down" the robot spins all the way round (and back) every chorus. | 108.5 BPM (checked), G minor | about 1.41 s after the song starts |  |
| `My_Type_Saint_Motel` | My Type - Saint Motel  (3:26) | Flirty retro disco: a strutting swagger walk, a hit on every horn stab in the chorus, and a solo full of spins. | 118 BPM (checked), C minor | about 0.67 s after the song starts |  |
| `Baba_Bandook_Ali_Azmat` | Baba Bandook - Ali Azmat & Haroon  (3:53) | The cartoon villain: a sneaky menacing creep in the verses, evil stomps, and a head-banging pump in the rock choruses. | 88.5 BPM (fairly sure), D major | about 0.3 s after the song starts |  |
| `Iraaday_Rovalio` | Iraaday - Rovalio & Abdul Hannan  (2:13) | Warm and dreamy: gentle sways and glides, soft figure-eights in the chorus and a slow spin to finish each line. | 90 BPM (checked), F# major | right when the song starts |  |
| `Worry_LONOWN` | worry - LONOWN (feat. riserayss)  (3:01) | TikTok edit style: sharp hits and freezes like video cuts, shimmy transitions, and bouncy grooves in the long instrumentals. | 99 BPM (fairly sure), B major | about 0.5 s after the song starts |  |
| `Sh_Boom_The_Chords_Cars` | Sh-Boom (Life Could Be a Dream) - The Chords  (2:26) | Cars - the neon cruise: slow, jazzy cruising sways and finger-snap looks like Lightning McQueen and Sally, a "ka-chow" pose, and a jazzy spin party for the sax solo. | 68.5 BPM (checked), F major | about 0.8 s after the song starts | Slow, jazzy cruising like the Cars neon-lights scene: one move per beat (the tempo here is half the song tempo on purpose). |
| `Ben_10_1_Classic_Theme` | Ben 10 Theme Song (original series opening) - Moxy (written by Andy Sturmer)  (1:01) | Ben 10 (2005): hero poses, the Omnitrix slam, transformation spins during the alien countdown, and a big hit on BEN 10! | 136 BPM (checked), D# minor | right when the song starts | The 1:01 TV opening. Works with the English AND the Hindi/Urdu version (same music and length). The title hit flashes BEN 10! on the screen. |
| `Ben_10_2_Alien_Force_Theme` | Ben 10: Alien Force Theme Song (Main Title / Opening) - Andy Sturmer (Cartoon Network)  (0:31) | Alien Force: older and serious - heavy stomps and hits, Omnitrix slams, and a transformation spin through the alien slideshow. | 135 BPM (checked), C minor | about 0.2 s after the song starts | The 0:31 TV opening. Works with the English AND the Hindi/Urdu version (same music and length). |
| `Ben_10_3_Ultimate_Alien_Theme` | Ben 10: Ultimate Alien Theme Song (Opening Theme) - Kristopher Carter, Michael McCuistion & Lolita Ritmanis (Cartoon Network)  (0:21) | Ultimate Alien: the Ultimatrix slam at the start, heavy hero stomps, a transformation spin, and a big pose when the logo hits. | 85 BPM (fairly sure), C minor | about 0.2 s after the song starts | The 0:21 TV opening. Works with the English AND the Hindi/Urdu version. The song is very fast (170), so the robot dances one move per beat. |
| `Ben_10_4_Omniverse_Theme` | Ben 10: Omniverse Theme Song (main opening) - Parry Gripp (writer and performer)  (0:20) | Omniverse: punchy hits on every "Ben 10!", an electric Feedback shake when FEEDBACK! flashes, and a Hero Time pose at the end. | 98 BPM (fairly sure), B minor | about 0.25 s after the song starts | The 0:21 TV opening. Works with the English AND the Hindi/Urdu version. FEEDBACK! flashes (with an electric zap) where Feedback appears. The song is very fast (196), so the robot dances one move per beat. |
| `Ben_10_5_Omniverse_Kid_Ben` | Ben 10: Omniverse Theme Song - Opening #1 (the version built around 11-year-old Ben, shown split-screen with 16-year-old Ben) - Parry Gripp (writer and performer)  (0:21) | Omniverse with kid Ben: bouncy hits for every "Ben 10!", an electric Feedback shake each time FEEDBACK! flashes, and a Hero Time pose to finish. | 98 BPM (fairly sure), B minor | about 0.2 s after the song starts | Opening #1 (Season 1 to S2E7 Bros in Space) - the one built around 11-year-old Ben, same song as the later Omniverse opening. FEEDBACK! flashes three times where kid-Ben Feedback appears. |
