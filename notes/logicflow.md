# Verarbeitung

Zwei Prozesse sind maßgeblich an der Erzeugung von Audiosignalen beteiligt.

1. Die Berechnung des spektralen Musters
2. Das Stimmen der Oszillatoren anhand dieses Musters.

Aktuell werden die Oszillatoren immer dann gestimmt, wenn die Berechnung des Musters erfolgt ist.

In Zukunft soll aber auch ein Timer bzw. ein Midi Takt vorgeben können, wann das Stimmen der Oszillatoren erfolgt.

Berechnung und Stimmen könnte ggf. voneinander getrennt werden.
So dass die FFT immer getriggered wird wenn der Buffer voll ist, aber die resultierende Map nur zur Berechnung genutzt
wird, wenn ein externer Trigger dazu eingeht.

Gibt's da Probleme?
-> Man könnte sich die Berechnung sparen wenn man den Befehl zur Berechnung auch nur dann gibt, wenn die FFT getriggert
werden soll. Aber das Problem ist, dass dann das Ergebnis der Map ggf. noch nicht zur Verfügung steht.

-> Alternative mit steady Berechnung ist mir glaub lieber. Performance overhead ist an der Stelle egal weil's im
Standardfall (ohne Taktgesteuertes Stimmen) ohnehin so läuft.

## Synchronisation

Die Oszillatoren dürfen nicht gestimmt werden, so lange die Berechnung noch läuft.
D.h. die Berechnung muss von einem Mutex geblockt werden. Für die Zeit der Berechnung dürfen die Oszis nicht gestimmt
werden.
Damit ist der Takt ist ggf. nicht 100% akkurat aber das wäre wsl. ohnehin nie der Fall, Verzögerung muss mal evaluiert
werden, ob's da hörbar Taktunstimmigkeiten gibt. 

Die maximale Verzögerung entspricht der Dauer der Berechnung einer Map.
