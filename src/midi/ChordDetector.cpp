#include "ChordDetector.h"
#include <QSet>
#include <QDebug>
#include <algorithm>

QString ChordDetector::detectChord(QList<int> notes) {
    if (notes.isEmpty()) {
        return QString();
    }

    // Remove duplicates and sort
    QSet<int> uniqueNotes = QSet<int>(notes.begin(), notes.end());
    notes = uniqueNotes.values();
    std::sort(notes.begin(), notes.end());

    // If only one note, return the note name
    if (notes.size() == 1) {
        return getNoteName(notes[0]);
    }

    // Normalize to single octave
    QList<int> normalizedNotes = normalizeToSingleOctave(notes);

    // Try each note as a potential root
    for (int i = 0; i < normalizedNotes.size(); i++) {
        QList<int> intervals = getIntervals(normalizedNotes);

        QString chordType = identifyChordType(intervals);
        if (!chordType.isEmpty()) {
            QString rootName = getNoteName(normalizedNotes[0]);
            return rootName + chordType;
        }

        // Rotate to try next note as root
        int temp = normalizedNotes.takeFirst();
        normalizedNotes.append(temp);
    }

    return QString();
}

QList<int> ChordDetector::normalizeToSingleOctave(QList<int> notes) {
    QList<int> normalized;
    for (int note : notes) {
        normalized.append(note % 12);
    }

    // Remove duplicates
    QSet<int> uniqueNotes = QSet<int>(normalized.begin(), normalized.end());
    normalized = uniqueNotes.values();
    std::sort(normalized.begin(), normalized.end());

    return normalized;
}

QString ChordDetector::getNoteName(int note) {
    static const QString noteNames[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    return noteNames[note % 12];
}

QList<int> ChordDetector::getIntervals(const QList<int>& normalizedNotes) {
    QList<int> intervals;
    if (normalizedNotes.isEmpty()) {
        return intervals;
    }

    int root = normalizedNotes[0];
    for (int i = 1; i < normalizedNotes.size(); i++) {
        int interval = (normalizedNotes[i] - root + 12) % 12;
        intervals.append(interval);
    }

    return intervals;
}

QString ChordDetector::identifyChordType(const QList<int>& intervals) {
    // Sort intervals for comparison
    QList<int> sortedIntervals = intervals;
    std::sort(sortedIntervals.begin(), sortedIntervals.end());

    // Debug output
    qDebug() << "\nnChord detection - Intervals:";
    for (int i = 0; i < sortedIntervals.size(); i++) {
        qDebug() << "  sortedIntervals[" << i << "] =" << sortedIntervals[i];
    }

    // 2-note chords (dyads/power chords)
    if (sortedIntervals.size() == 1) {
        if (sortedIntervals[0] == 5) return " (5th)"; // Power chord
        if (sortedIntervals[0] == 7) return " (5th)"; // Perfect fifth
        if (sortedIntervals[0] == 3) return "m"; // Minor third
        if (sortedIntervals[0] == 4) return ""; // Major third
    }

    // 3-note chords (triads)
    if (sortedIntervals.size() == 2) {
        if (sortedIntervals[0] == 3 && sortedIntervals[1] == 7) return "m"; // Minor
        if (sortedIntervals[0] == 4 && sortedIntervals[1] == 7) return ""; // Major
        if (sortedIntervals[0] == 3 && sortedIntervals[1] == 6) return "dim"; // Diminished
        if (sortedIntervals[0] == 4 && sortedIntervals[1] == 8) return "aug"; // Augmented
        if (sortedIntervals[0] == 2 && sortedIntervals[1] == 7) return "sus2"; // Sus2
        if (sortedIntervals[0] == 5 && sortedIntervals[1] == 7) return "sus4"; // Sus4
    }

    // 4-note chords (seventh chords)
    if (sortedIntervals.size() == 3) {
        if (sortedIntervals[0] == 4 && sortedIntervals[1] == 7 && sortedIntervals[2] == 11) return "maj7"; // Major 7th
        if (sortedIntervals[0] == 3 && sortedIntervals[1] == 7 && sortedIntervals[2] == 10) return "m7"; // Minor 7th
        if (sortedIntervals[0] == 4 && sortedIntervals[1] == 7 && sortedIntervals[2] == 10) return "7"; // Dominant 7th
        if (sortedIntervals[0] == 3 && sortedIntervals[1] == 6 && sortedIntervals[2] == 10) return "m7b5"; // Half-diminished
        if (sortedIntervals[0] == 3 && sortedIntervals[1] == 6 && sortedIntervals[2] == 9) return "dim7"; // Diminished 7th
        if (sortedIntervals[0] == 3 && sortedIntervals[1] == 7 && sortedIntervals[2] == 11) return "mmaj7"; // Minor major 7th
        if (sortedIntervals[0] == 4 && sortedIntervals[1] == 8 && sortedIntervals[2] == 10) return "aug7"; // Augmented 7th
        if (sortedIntervals[0] == 4 && sortedIntervals[1] == 8 && sortedIntervals[2] == 11) return "augmaj7"; // Augmented major 7th
    }

    // Extended chords (9th, 11th, 13th)
    if (sortedIntervals.size() == 4) {
        if (sortedIntervals[0] == 2 && sortedIntervals[1] == 4 && sortedIntervals[2] == 7 && sortedIntervals[3] == 10) return "9"; // Dominant 9th
        if (sortedIntervals[0] == 2 && sortedIntervals[1] == 4 && sortedIntervals[2] == 7 && sortedIntervals[3] == 11) return "maj9"; // Major 9th
        if (sortedIntervals[0] == 2 && sortedIntervals[1] == 3 && sortedIntervals[2] == 7 && sortedIntervals[3] == 10) return "m9"; // Minor 9th
        if (sortedIntervals[0] == 1 && sortedIntervals[1] == 4 && sortedIntervals[2] == 7 && sortedIntervals[3] == 10) return "7b9"; // Dominant 7th flat 9
        if (sortedIntervals[0] == 3 && sortedIntervals[1] == 4 && sortedIntervals[2] == 7 && sortedIntervals[3] == 10) return "7#9"; // Dominant 7th sharp 9
    }

    return QString();
}
