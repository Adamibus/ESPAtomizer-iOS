//
//  HistoryStore.swift
//  ESPAtomizer-iOS
//
//  Fix (Phase 2 — durable local history): the in-memory `history` rolling window used to
//  vanish on app restart. This is a deliberately lightweight JSON-file store (Documents dir),
//  not SwiftData/Core Data, so it needs no model schema/migration to get real persistence
//  shipped now. See README "Backend & architecture analysis" for the upgrade path.
//

import Foundation

/// One connected run: start/end time plus the temperature/setpoint/output samples taken during it.
struct HistorySession: Identifiable, Codable {
    var id = UUID()
    var startedAt: Date
    var endedAt: Date?
    var points: [AtomizerViewModel.DataPoint]
}

/// Persists session logs to a JSON file in the app's Documents directory.
/// Writes happen on a background queue; `save` is safe to call frequently.
final class HistoryStore {
    private let fileURL: URL
    private let maxSessions: Int
    private let maxPointsPerSession: Int
    private let queue = DispatchQueue(label: "com.espatomizer.historystore", qos: .utility)

    init(maxSessions: Int = 30, maxPointsPerSession: Int = 5000) {
        let dir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
        self.fileURL = dir.appendingPathComponent("history.json")
        self.maxSessions = maxSessions
        self.maxPointsPerSession = maxPointsPerSession
    }

    /// Synchronous load — call once at startup only.
    func load() -> [HistorySession] {
        guard let data = try? Data(contentsOf: fileURL) else { return [] }
        return (try? JSONDecoder().decode([HistorySession].self, from: data)) ?? []
    }

    /// Bounds session/point counts, then writes off the main thread.
    func save(_ sessions: [HistorySession]) {
        let trimmed: [HistorySession] = sessions.suffix(maxSessions).map { session in
            var session = session
            if session.points.count > maxPointsPerSession {
                session.points = Array(session.points.suffix(maxPointsPerSession))
            }
            return session
        }
        let url = fileURL
        queue.async {
            guard let data = try? JSONEncoder().encode(trimmed) else { return }
            try? data.write(to: url, options: .atomic)
        }
    }
}
