import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 900
    height: 650
    visible: false   // γίνεται true στο Component.onCompleted (εκτός αν η αυτόματη εκτέλεση παραλείπεται)
    title: "Rate 24 - Productivity Tracker"
    color: "#050805"

    // ---- HACKER THEME (μόνο εμφάνιση) ----
    // Επιλογή πραγματικής monospace γραμματοσειράς (το Canvas δεν δέχεται το generic "Monospace")
    function pickMono() {
        if (typeof Qt.fontFamilies !== "function")
            return ""
        let have = Qt.fontFamilies()
        let prefs = ["JetBrains Mono", "Fira Code", "Hack", "Source Code Pro", "DejaVu Sans Mono",
                     "Liberation Mono", "Noto Sans Mono", "Ubuntu Mono", "Consolas", "Menlo",
                     "Courier New", "Courier"]
        for (let i = 0; i < prefs.length; i++) {
            if (have.indexOf(prefs[i]) >= 0)
                return prefs[i]
        }
        for (let j = 0; j < have.length; j++) {
            if (have[j].toLowerCase().indexOf("mono") >= 0)
                return have[j]
        }
        return ""
    }
    readonly property string monoPicked: pickMono()
    readonly property string mono: monoPicked !== "" ? monoPicked : "Monospace"
    readonly property string canvasMono: monoPicked !== "" ? monoPicked : Qt.application.font.family
    readonly property color neon: "#00ff41"
    readonly property color neonDim: "#00a82b"
    readonly property color neonDark: "#0b2a0f"
    readonly property color panel: "#0a120a"
    readonly property color warn: "#ff3131"
    readonly property color amber: "#ffb000"
    readonly property color cyan: "#00e5ff"
    readonly property color emptyBar: "#26302a"

    // Φόντο: ζωντανό "matrix rain" (Canvas + Timer) + σκούρο overlay για ευανάγνωστο κείμενο
    background: Item {
        Canvas {
            id: rain
            anchors.fill: parent

            readonly property int cell: 18
            readonly property string glyphs: "01abcdef0123456789<>{}[]/\\|;:+=*#$%&@"
            property var dropRows: []     // τρέχουσα γραμμή (κεφάλι) ανά στήλη
            property var dropSpeeds: []   // ταχύτητα ανά στήλη (γραμμές ανά frame)
            property bool cleared: false

            function randomGlyph() {
                return glyphs.charAt(Math.floor(Math.random() * glyphs.length))
            }

            function resetDrops() {
                let cols = Math.ceil(width / cell)
                let r = []
                let s = []
                for (let i = 0; i < cols; i++) {
                    r.push(-Math.random() * 60)
                    s.push(0.25 + Math.random() * 0.75)
                }
                dropRows = r
                dropSpeeds = s
                cleared = false
            }

            onWidthChanged: resetDrops()
            onHeightChanged: cleared = false
            Component.onCompleted: resetDrops()

            Timer {
                interval: 50          // ~20 fps
                repeat: true
                running: root.visibility !== Window.Minimized && root.visibility !== Window.Hidden && rain.available
                onTriggered: rain.requestPaint()
            }

            onPaint: {
                let ctx = getContext("2d")
                if (!cleared) {
                    ctx.fillStyle = "#050805"
                    ctx.fillRect(0, 0, width, height)
                    cleared = true
                }
                // Σβήσιμο: κάθε frame "σκουραίνει" ό,τι υπάρχει → δημιουργεί την ουρά
                ctx.fillStyle = "rgba(5, 8, 5, 0.09)"
                ctx.fillRect(0, 0, width, height)

                ctx.font = (cell - 2) + "px '" + root.canvasMono + "'"
                ctx.textBaseline = "top"
                let maxRow = Math.ceil(height / cell)

                for (let i = 0; i < dropRows.length; i++) {
                    let prev = Math.floor(dropRows[i])
                    dropRows[i] += dropSpeeds[i]
                    let cur = Math.floor(dropRows[i])
                    if (cur === prev)
                        continue
                    let x = i * cell

                    // Το προηγούμενο κεφάλι γίνεται πράσινο σύμβολο της ουράς
                    if (prev >= 0 && prev <= maxRow) {
                        ctx.fillStyle = "#050805"
                        ctx.fillRect(x, prev * cell, cell, cell)
                        ctx.fillStyle = "rgba(0, 255, 65, 0.65)"
                        ctx.fillText(randomGlyph(), x + 2, prev * cell)
                    }
                    // Νέο, φωτεινό κεφάλι
                    if (cur >= 0 && cur <= maxRow) {
                        ctx.fillStyle = "#050805"
                        ctx.fillRect(x, cur * cell, cell, cell)
                        ctx.fillStyle = "#c8ffd4"
                        ctx.fillText(randomGlyph(), x + 2, cur * cell)
                    }
                    // Όταν βγει από την οθόνη, ξαναρχίζει από πάνω (τυχαία)
                    if (cur > maxRow && Math.random() > 0.975) {
                        dropRows[i] = -Math.random() * 20
                        dropSpeeds[i] = 0.25 + Math.random() * 0.75
                    }
                }
            }
        }
        Rectangle {
            anchors.fill: parent
            color: "#050805"
            opacity: 0.5
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 12
            color: "#99050805"
            border.color: "#0b2a0f"
            border.width: 1
        }
    }

    // Κουμπί σε στυλ τερματικού
    component HackerButton: Button {
        id: hb
        font.family: root.mono
        focusPolicy: Qt.StrongFocus   // επιτρέπει Tab
        // Enter / Return πατάνε το κουμπί που έχει focus
        Keys.onReturnPressed: (event) => { if (!event.isAutoRepeat) hb.clicked() }
        Keys.onEnterPressed: (event) => { if (!event.isAutoRepeat) hb.clicked() }
        contentItem: Text {
            text: "[ " + hb.text + " ]"
            font: hb.font
            color: hb.down ? "#000000" : (hb.hovered || hb.activeFocus ? "#ffffff" : root.neon)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            implicitHeight: 40
            implicitWidth: 120
            color: hb.down ? root.neon : (hb.hovered ? root.neonDark : "transparent")
            border.color: hb.activeFocus ? "#ffffff" : root.neon
            border.width: hb.activeFocus ? 2 : 1
        }
    }

    // Πεδίο κειμένου σε στυλ τερματικού
    component HackerField: TextField {
        id: hf
        font.family: root.mono
        font.pixelSize: 18
        color: root.neon
        placeholderTextColor: root.neonDim
        selectionColor: root.neon
        selectedTextColor: "#000000"
        implicitWidth: 220
        cursorDelegate: Rectangle {
            width: 10
            color: root.neon
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                running: hf.activeFocus
                NumberAnimation { to: 0; duration: 500 }
                NumberAnimation { to: 1; duration: 500 }
            }
        }
        background: Rectangle {
            color: hf.enabled ? root.panel : "#050805"
            border.color: hf.activeFocus ? root.neon : root.neonDim
            border.width: 1
        }
    }

    // Μία γραμμή διαγράμματος: ημερομηνία | ▰▰▰▱▱▱ | σκορ  (ίδια φιλοσοφία με το CLI)
    component TermBarRow: RowLayout {
        id: tb
        property string date: ""
        property real score: 0
        property int blocks: 0
        property int level: 0      // 2 = πράσινο, 1 = κίτρινο, 0 = κόκκινο
        property int barLength: 35
        readonly property color levelColor: level === 2 ? root.neon : (level === 1 ? root.amber : root.warn)
        spacing: 8

        Text {
            text: tb.date
            color: root.neonDim
            font.family: root.mono
            font.pixelSize: 14
            Layout.preferredWidth: 100
        }
        Text { text: "|"; color: root.neonDark; font.family: root.mono; font.pixelSize: 14 }
        Row {
            spacing: 2
            Repeater {
                model: tb.barLength
                Rectangle {
                    width: 9
                    height: 14
                    radius: 1
                    color: index < tb.blocks ? tb.levelColor : root.emptyBar
                }
            }
        }
        Text { text: "|"; color: root.neonDark; font.family: root.mono; font.pixelSize: 14 }
        Text {
            text: tb.score.toFixed(1)
            color: tb.levelColor
            font.family: root.mono
            font.pixelSize: 14
            font.bold: true
            Layout.preferredWidth: 64
        }
    }

    // Σε λειτουργία "Browse History" (χωρίς να έχει σωθεί η σημερινή μέρα) το κλείσιμο ΔΕΝ γράφει μηδενικά
    readonly property bool browsing: chartForm.fromInput && (chartForm.visible || dayForm.visible)

    // Κρατά τον κέρσορα ορατό όταν γράφεις σε TextArea που βρίσκεται μέσα σε ScrollView
    function ensureCursorVisible(scroll, field) {
        Qt.callLater(function() {
            let flick = scroll.contentItem
            if (!flick || !field)
                return
            let r = field.cursorRectangle
            let pt = field.mapToItem(flick.contentItem, r.x, r.y)
            let margin = 16
            let maxY = Math.max(0, flick.contentHeight - flick.height)
            let target = flick.contentY
            if (pt.y < flick.contentY + margin)
                target = pt.y - margin
            else if (pt.y + r.height > flick.contentY + flick.height - margin)
                target = pt.y + r.height - flick.height + margin
            flick.contentY = Math.max(0, Math.min(maxY, target))
        })
    }

    // Άνοιγμα ιστορικού/διαγραμμάτων χωρίς να χρειαστεί να συμπληρωθεί η σημερινή μέρα
    function openBrowse() {
        chartForm.fromInput = true
        inputForm.visible = false
        sdForm.visible = false
        chartForm.showActivities = false
        chartForm.visible = true
        chartForm.loadChart()
    }

    function closeBrowse() {
        chartForm.visible = false
        chartForm.fromInput = false
        refreshUI()
    }

    function refreshUI() {
        if (Rate_24_Backend.isFinished()) {
            activityNameText.text = "Day Completed!"
            inputForm.visible = false
            sdForm.visible = false
            summaryForm.visible = true

            finalScoreText.text = "Final Score: " + Rate_24_Backend.calculateFinalScore().toFixed(1) + " / 100"
            detailedScoresText.text = Rate_24_Backend.getDetailedScores()
            summaryForm.loadNotesTemplate(Rate_24_Backend.getNotesTemplate())
            summaryForm.showBests(Rate_24_Backend.getBestInfo())
        } else {
            activityNameText.text = "Activity: " + Rate_24_Backend.getCurrentActivityName()
            inputForm.visible = true
            sdForm.visible = false
            realTimeInput.forceActiveFocus() // Reset focus to the first text field
        }
    }

    // Esc: βήμα πίσω (SD φόρμα → πεδία, acknowledge → πεδία, αλλιώς undo της προηγούμενης δραστηριότητας)
    function goBack() {
        if (dayForm.visible) {
            dayForm.closeDay()
            return
        }
        if (chartForm.visible) {
            if (chartForm.showActivities) {
                chartForm.showActivities = false
            } else if (chartForm.fromInput) {
                root.closeBrowse()
            }
            return
        }
        if (sdForm.visible) {
            sdForm.visible = false
            inputForm.visible = true
            realTimeInput.forceActiveFocus()
            return
        }
        if (inputForm.waitingForAck) {
            warningText.text = ""
            inputForm.waitingForAck = false
            realTimeInput.forceActiveFocus()
            return
        }
        // Στο summary: δεν κάνουμε undo αν έχουν γραφτεί σημειώσεις (για να μη χαθούν)
        if (summaryForm.visible && !summaryForm.notesAreEmpty())
            return
        if (Rate_24_Backend.undoLast()) {
            inputForm.resetForm()
            summaryForm.visible = false
            refreshUI()
        }
    }

    // Όταν κλείνει το πρόγραμμα (X ή Qt.quit): αν η ημέρα δεν έχει σωθεί, καταγράφεται με μηδενικά
    Connections {
        target: Qt.application
        function onAboutToQuit() {
            if (!root.browsing)
                Rate_24_Backend.finalizeOnExit()
        }
    }

    Shortcut {
        sequence: "Ctrl+H"
        enabled: inputForm.visible
        onActivated: root.openBrowse()
    }

    Shortcut {
        sequence: "Ctrl+S"
        enabled: dayForm.visible
        onActivated: dayForm.saveEdits()
    }

    Shortcut {
        sequence: "Escape"
        enabled: !chartForm.visible || chartForm.showActivities || chartForm.fromInput
        onActivated: root.goBack()
    }

    Component.onCompleted: {
        // Αυτόματη εκκίνηση (--auto) αλλά η μέρα έχει ήδη σωθεί: κλείνουμε αθόρυβα, χωρίς να εμφανιστεί παράθυρο
        if (Rate_24_Backend.shouldSkipAutoLaunch()) {
            Qt.quit()
            return
        }
        root.visible = true
        refreshUI()
        let n = Rate_24_Backend.restoredCount()
        if (n > 0)
            restoreNotice.text = "[draft restored: " + n + " / " + Rate_24_Backend.getActivityCount() + " activities already logged]"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 25

        Text {
            text: "root@rate24:~$ ./productivity_tracker --track        [Esc = undo]   [Ctrl+H = history]"
            color: root.neonDim
            font.family: root.mono
            font.pixelSize: 14
            Layout.alignment: Qt.AlignLeft
        }

        Text {
            id: restoreNotice
            text: ""
            color: root.amber
            font.family: root.mono
            font.pixelSize: 13
            Layout.alignment: Qt.AlignLeft
            visible: text !== ""
        }

        Text {
            id: activityNameText
            text: "Loading..."
            color: root.neon
            font.family: root.mono
            font.pixelSize: 28
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        // --- 1. INPUT FORM ---
        ColumnLayout {
            id: inputForm
            spacing: 25
            Layout.alignment: Qt.AlignHCenter

            property bool waitingForAck: false
            property int tempRt: 0
            property int tempQ: 0
            // Αν ο χρόνος είναι 0, η ποιότητα δεν μετράει καθόλου
            readonly property bool timeIsZero: realTimeInput.text !== "" && parseInt(realTimeInput.text) === 0

            Text {
                id: warningText
                text: ""
                color: root.warn
                font.family: root.mono
                font.pixelSize: 18
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                visible: text !== ""
            }

            RowLayout {
                spacing: 20
                Text { text: "> Time (mins):"; color: root.neonDim; font.family: root.mono; font.pixelSize: 18 }
                HackerField {
                    id: realTimeInput
                    placeholderText: "e.g. 120"
                    enabled: !inputForm.waitingForAck
                    maximumLength: 4
                    validator: RegularExpressionValidator { regularExpression: /^[0-9]+$/ }
                    onAccepted: {
                        if (inputForm.timeIsZero)
                            submitBtn.clicked()      // χρόνος 0 → δεν χρειάζεται ποιότητα
                        else
                            qualityInput.forceActiveFocus()
                    }
                }
            }

            RowLayout {
                spacing: 20
                Text { text: "> Quality (0-100):"; color: root.neonDim; font.family: root.mono; font.pixelSize: 18 }
                HackerField {
                    id: qualityInput
                    placeholderText: inputForm.timeIsZero ? "ignored (time = 0)" : "e.g. 85"
                    enabled: !inputForm.waitingForAck && !inputForm.timeIsZero
                    maximumLength: 3
                    validator: IntValidator { bottom: 0; top: 100 }
                    onAccepted: submitBtn.clicked()
                }
            }

            HackerButton {
                id: submitBtn
                text: inputForm.waitingForAck ? "Acknowledge & Continue" : "Submit Activity"
                font.pixelSize: 18
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20

                // Το Enter (ή Return) το χειρίζεται πλέον το HackerButton

                onClicked: {
                    if (inputForm.waitingForAck) {
                        Rate_24_Backend.commitActivity(inputForm.tempRt, inputForm.tempQ, false)
                        inputForm.resetForm()
                        refreshUI()
                        return
                    }

                    if (realTimeInput.text !== "" && (qualityInput.text !== "" || inputForm.timeIsZero)) {
                        let rt = Math.min(1440, parseInt(realTimeInput.text))   // max 24h
                        let q = inputForm.timeIsZero ? 0 : Math.max(0, Math.min(100, parseInt(qualityInput.text)))
                        let warnings = Rate_24_Backend.getWarningMessages(rt, q)

                        if (Rate_24_Backend.checkNeedsSD(rt, q)) {
                            sdForm.currentRt = rt
                            sdForm.currentQ = q
                            sdForm.currentWarning = warnings
                            inputForm.visible = false
                            sdForm.visible = true
                        } else if (warnings !== "") {
                            warningText.text = warnings
                            inputForm.tempRt = rt
                            inputForm.tempQ = q
                            inputForm.waitingForAck = true
                            // Δίνουμε το focus στο κουμπί για να δουλέψει αμέσως το Enter!
                            submitBtn.forceActiveFocus()
                        } else {
                            Rate_24_Backend.commitActivity(rt, q, false)
                            inputForm.resetForm()
                            refreshUI()
                        }
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                HackerButton {
                    text: "Browse History"
                    font.pixelSize: 16
                    onClicked: root.openBrowse()
                }
                HackerButton {
                    text: "Export TXT"
                    font.pixelSize: 16
                    onClicked: {
                        let path = Rate_24_Backend.exportToText()
                        startStatus.color = path !== "" ? root.neon : root.warn
                        startStatus.text = path !== "" ? "exported: " + path : "[ERROR] export failed"
                    }
                }
            }
            Text {
                id: startStatus
                text: ""
                color: root.neon
                font.family: root.mono
                font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
                visible: text !== ""
            }

            function resetForm() {
                realTimeInput.text = ""
                qualityInput.text = ""
                warningText.text = ""
                inputForm.waitingForAck = false
                restoreNotice.text = ""
            }
        }

        // --- 2. SD / TO FORM ---
        ColumnLayout {
            id: sdForm
            visible: false
            spacing: 20
            Layout.alignment: Qt.AlignHCenter

            property int currentRt: 0
            property int currentQ: 0
            property string currentWarning: ""

            // Όταν εμφανιστεί η φόρμα, το focus πάει στο πρώτο κουμπί (SD)
            onVisibleChanged: if (visible) sdBtn.forceActiveFocus()

            // Εδώ εμφανίζεται τι πήγε στραβά
            Text {
                text: sdForm.currentWarning
                color: root.warn
                font.family: root.mono
                font.pixelSize: 18
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                visible: text !== ""
            }

            // Η ερώτηση SD / TO
            Text {
                text: "I want you to consider seriously if it was a strategic decision (SD)\nor just a tragic oversight... (TO):"
                color: root.amber
                font.family: root.mono
                font.pixelSize: 20
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                spacing: 30
                Layout.alignment: Qt.AlignHCenter

                HackerButton {
                    id: sdBtn
                    text: "Strategic Decision (SD)"
                    font.pixelSize: 16
                    KeyNavigation.tab: toBtn
                    KeyNavigation.backtab: toBtn
                    KeyNavigation.right: toBtn
                    KeyNavigation.left: toBtn
                    onClicked: {
                        Rate_24_Backend.commitActivity(sdForm.currentRt, sdForm.currentQ, true)
                        sdForm.cleanupAndNext()
                    }
                }
                HackerButton {
                    id: toBtn
                    text: "Tragic Oversight (TO)"
                    font.pixelSize: 16
                    KeyNavigation.tab: sdBtn
                    KeyNavigation.backtab: sdBtn
                    KeyNavigation.right: sdBtn
                    KeyNavigation.left: sdBtn
                    onClicked: {
                        Rate_24_Backend.commitActivity(sdForm.currentRt, sdForm.currentQ, false)
                        sdForm.cleanupAndNext()
                    }
                }
            }

            function cleanupAndNext() {
                inputForm.resetForm()
                refreshUI()
            }
        }

        // --- 3. SUMMARY FORM ---
        ColumnLayout {
            id: summaryForm
            visible: false
            spacing: 20
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Οι τίτλοι των δραστηριοτήτων (όπως τους επιστρέφει το getNotesTemplate, π.χ. "Piano Studying:")
            property var noteTitles: []

            // Σπάει το template σε τίτλους: κάθε μη κενή γραμμή είναι τίτλος δραστηριότητας
            function loadNotesTemplate(tpl) {
                let lines = tpl.split("\n")
                let titles = []
                for (let i = 0; i < lines.length; i++) {
                    if (lines[i].trim() !== "")
                        titles.push(lines[i])
                }
                noteTitles = titles
                Qt.callLater(function() { summaryForm.focusNote(0) })
            }

            // Ξανα-συνθέτει ΑΚΡΙΒΩΣ τη μορφή "Τίτλος:\nσημειώσεις\n\n" που περιμένει το saveDay
            function assembleNotes() {
                let out = ""
                for (let i = 0; i < notesRepeater.count; i++) {
                    let card = notesRepeater.itemAt(i)
                    if (!card) continue;

                    let note = card.noteText.replace(/\s+$/, "")
                    if (note === "")
                        out += card.title + "\n\n"
                    else
                        out += card.title + "\n" + note + "\n\n"
                }
                return out
            }

            // Οι σημειώσεις ανά δραστηριότητα (ίδια σειρά με τις δραστηριότητες του backend)
            function assembleNoteList() {
                let list = []
                for (let i = 0; i < notesRepeater.count; i++) {
                    let card = notesRepeater.itemAt(i)
                    list.push(card ? card.noteText.replace(/\s+$/, "") : "")
                }
                return list
            }

            function notesAreEmpty() {
                for (let i = 0; i < notesRepeater.count; i++) {
                    let card = notesRepeater.itemAt(i)
                    if (card && card.noteText.trim() !== "")
                        return false
                }
                return true
            }

            // Μηνύματα προσωπικού ρεκόρ (συνολικό σκορ ημέρας και ανά δραστηριότητα)
            function showBests(info) {
                if (info.dayBest) {
                    dayBestText.text = "★ NEW PERSONAL BEST!  " + info.score.toFixed(1)
                                     + "   (previous best " + info.prevDay.toFixed(1)
                                     + ", +" + (info.score - info.prevDay).toFixed(1) + ")"
                } else {
                    dayBestText.text = ""
                }
                let lines = []
                for (let i = 0; i < info.activities.length; i++) {
                    let a = info.activities[i]
                    if (a.best)
                        lines.push("★ " + a.name + ": " + a.score.toFixed(1) + "  (prev " + a.prev.toFixed(1) + ")")
                }
                activityBestText.text = lines.join("\n")
            }

            // Tab / Shift+Tab ανάμεσα στα πεδία σημειώσεων και στο τέλος στα κουμπιά
            function focusNote(i) {
                if (i < 0) i = 0
                if (i >= notesRepeater.count) {
                    saveDayBtn.forceActiveFocus()
                } else {
                    notesRepeater.itemAt(i).field.forceActiveFocus()
                }
            }

            Text {
                id: finalScoreText
                text: ""
                color: root.cyan
                font.family: root.mono
                font.pixelSize: 26
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            // Μήνυμα ρεκόρ ημέρας (αναβοσβήνει ελαφρά)
            Text {
                id: dayBestText
                text: ""
                color: root.amber
                font.family: root.mono
                font.pixelSize: 20
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                visible: text !== ""
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: dayBestText.visible
                    NumberAnimation { to: 0.55; duration: 700 }
                    NumberAnimation { to: 1.0; duration: 700 }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 30

                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 280

                    Text { text: "# Activity Breakdown:"; color: root.neonDim; font.family: root.mono; font.pixelSize: 18; font.bold: true }
                    Text {
                        id: detailedScoresText
                        color: root.neon
                        font.family: root.mono
                        font.pixelSize: 16
                        lineHeight: 1.5
                    }

                    Text {
                        text: "# New bests:"
                        color: root.amber
                        font.family: root.mono
                        font.pixelSize: 16
                        font.bold: true
                        Layout.topMargin: 10
                        visible: activityBestText.text !== ""
                    }
                    Text {
                        id: activityBestText
                        text: ""
                        color: root.amber
                        font.family: root.mono
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        visible: text !== ""
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100   // το περιεχόμενο δεν επηρεάζει το πλάτος: δεν σπρώχνει το breakdown αριστερά
                    Layout.minimumWidth: 0
                    Layout.fillHeight: true

                    Text { text: "# Daily Notes & Reflections:"; color: root.neonDim; font.family: root.mono; font.pixelSize: 18; font.bold: true }

                    ScrollView {
                        id: notesScroll
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ScrollBar.vertical: ScrollBar {
                            contentItem: Rectangle { implicitWidth: 6; radius: 3; color: root.neonDim }
                            background: Rectangle { color: "transparent" }
                        }

                        ColumnLayout {
                            width: notesScroll.availableWidth
                            spacing: 14

                            Repeater {
                                id: notesRepeater
                                model: summaryForm.noteTitles

                                delegate: ColumnLayout {
                                    id: card
                                    Layout.fillWidth: true
                                    spacing: 0

                                    property string title: modelData
                                    property alias field: noteField
                                    property string noteText: noteField.text

                                    // Κλειδωμένος τίτλος (read-only, δεν είναι πεδίο κειμένου)
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 30
                                        color: root.neonDark
                                        border.color: root.neonDim
                                        border.width: 1

                                        Text {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "> " + card.title.replace(/:\s*$/, "")
                                            color: root.neon
                                            font.family: root.mono
                                            font.pixelSize: 16
                                            font.bold: true
                                        }
                                        Text {
                                            anchors.right: parent.right
                                            anchors.rightMargin: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: "[ LOCKED ]"
                                            color: root.neonDim
                                            font.family: root.mono
                                            font.pixelSize: 12
                                        }
                                    }

                                    // Πεδίο σημειώσεων για αυτή τη δραστηριότητα
                                    TextArea {
                                        id: noteField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: Math.max(64, implicitHeight)
                                        color: root.neon
                                        font.family: root.mono
                                        font.pixelSize: 16
                                        wrapMode: TextArea.WrapAtWordBoundaryOrAnywhere   // σπάει και μακριές λέξεις/χαρακτήρες χωρίς κενά
                                        placeholderText: "// notes..."
                                        placeholderTextColor: root.neonDim
                                        selectionColor: root.neon
                                        selectedTextColor: "#000000"
                                        leftPadding: 10
                                        topPadding: 8
                                        onCursorRectangleChanged: if (activeFocus) root.ensureCursorVisible(notesScroll, noteField)
                                        onActiveFocusChanged: if (activeFocus) root.ensureCursorVisible(notesScroll, noteField)
                                        Keys.onTabPressed: (event) => { event.accepted = true; summaryForm.focusNote(index + 1) }
                                        Keys.onBacktabPressed: (event) => { event.accepted = true; summaryForm.focusNote(index - 1) }
                                        background: Rectangle {
                                            color: root.panel
                                            border.color: noteField.activeFocus ? root.neon : root.neonDim
                                            border.width: noteField.activeFocus ? 2 : 1
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                HackerButton {
                    id: saveDayBtn
                    text: "Save Day & Exit"
                    font.pixelSize: 18
                    onClicked: {
                        Rate_24_Backend.saveDay(summaryForm.assembleNotes(), summaryForm.assembleNoteList())
                        Qt.quit()
                    }
                }

                HackerButton {
                    id: chartBtn
                    text: "View Progress Chart"
                    font.pixelSize: 18
                    onClicked: {
                        Rate_24_Backend.saveDay(summaryForm.assembleNotes(), summaryForm.assembleNoteList()) // Σώζουμε πρώτα τη μέρα
                        summaryForm.visible = false
                        chartForm.fromInput = false
                        chartForm.visible = true
                        chartForm.loadChart() // Φορτώνουμε τα δεδομένα
                    }
                }
            }
        }

        // --- 4. CHART FORM (terminal bars, όπως στο CLI) ---
        ColumnLayout {
            id: chartForm
            visible: false
            spacing: 8
            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property int barLength: 35
            property var rows: []          // γραμμές συνολικού διαγράμματος
            property var actRows: []       // επίπεδη λίστα γραμμών για τα διαγράμματα ανά δραστηριότητα
            property bool showActivities: false
            property bool fromInput: false     // ανοίχτηκε από την αρχική οθόνη (browse mode), όχι μετά από Save

            // Μόλις εμφανιστεί το διάγραμμα, το focus πάει στο πεδίο των ημερών
            onVisibleChanged: if (visible) daysInput.forceActiveFocus()

            // Μενού επιλογής ημερών
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 15
                Layout.topMargin: 10

                Text {
                    text: "> Show last (days, 0 for all):"
                    color: root.neonDim
                    font.family: root.mono
                    font.pixelSize: 18
                }

                SpinBox {
                    id: daysFilter
                    from: 0
                    to: 365
                    value: 14 // Προεπιλογή 14 μέρες
                    editable: true  // επιτρέπει πληκτρολόγηση του αριθμού
                    validator: IntValidator { bottom: daysFilter.from; top: daysFilter.to }
                    font.pixelSize: 16
                    font.family: root.mono
                    // Ανανεώνει το γράφημα δυναμικά κάθε φορά που αλλάζεις τον αριθμό
                    onValueChanged: if (chartForm.visible) chartForm.loadChart()

                    contentItem: TextInput {
                        id: daysInput
                        text: daysFilter.textFromValue(daysFilter.value, daysFilter.locale)
                        font: daysFilter.font
                        color: root.neon
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        readOnly: !daysFilter.editable
                        validator: daysFilter.validator
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        selectByMouse: true
                        selectionColor: root.neon
                        selectedTextColor: "#000000"
                        onActiveFocusChanged: if (activeFocus) selectAll()
                        // Enter: οριστικοποίηση του αριθμού και άμεση ανανέωση του διαγράμματος
                        onAccepted: {
                            let v = parseInt(text)
                            if (isNaN(v)) {
                                text = Qt.binding(function() {
                                    return daysFilter.textFromValue(daysFilter.value, daysFilter.locale)
                                })
                                return
                            }
                            daysFilter.value = Math.max(daysFilter.from, Math.min(daysFilter.to, v))
                            chartForm.loadChart()
                            selectAll()
                        }
                    }
                    up.indicator: Rectangle {
                        x: daysFilter.mirrored ? 0 : parent.width - width
                        height: parent.height
                        implicitWidth: 36
                        implicitHeight: 36
                        color: daysFilter.up.pressed ? root.neon : "transparent"
                        border.color: root.neonDim
                        Text {
                            text: "+"
                            anchors.centerIn: parent
                            color: daysFilter.up.pressed ? "#000000" : root.neon
                            font.family: root.mono
                            font.pixelSize: 18
                        }
                    }
                    down.indicator: Rectangle {
                        x: daysFilter.mirrored ? parent.width - width : 0
                        height: parent.height
                        implicitWidth: 36
                        implicitHeight: 36
                        color: daysFilter.down.pressed ? root.neon : "transparent"
                        border.color: root.neonDim
                        Text {
                            text: "-"
                            anchors.centerIn: parent
                            color: daysFilter.down.pressed ? "#000000" : root.neon
                            font.family: root.mono
                            font.pixelSize: 18
                        }
                    }
                    background: Rectangle {
                        implicitWidth: 140
                        color: root.panel
                        border.color: root.neonDim
                    }
                }

                Text {
                    id: chartInfo
                    text: ""
                    color: root.cyan
                    font.family: root.mono
                    font.pixelSize: 14
                }
            }

            // ================= ΣΕΛΙΔΑ 1: ΣΥΝΟΛΙΚΟ ΣΚΟΡ ΗΜΕΡΑΣ =================
            Text {
                text: "=".repeat(58)
                color: root.neonDim
                font.family: root.mono
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 6
                visible: !chartForm.showActivities
            }
            Text {
                id: chartTitle
                text: "TOTAL DAY SCORE EVOLUTION"
                color: root.neon
                font.family: root.mono
                font.pixelSize: 18
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                visible: !chartForm.showActivities
            }
            Text {
                text: "=".repeat(58)
                color: root.neonDim
                font.family: root.mono
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
                visible: !chartForm.showActivities
            }

            // Οι γραμμές του συνολικού διαγράμματος (virtualized: ΟΚ ακόμα και για 365 μέρες)
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !chartForm.showActivities

                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(parent.width, 640)
                    color: "#cc050805"
                    border.color: root.neonDark
                    border.width: 1

                    ListView {
                        id: chartList
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true
                        spacing: 2
                        model: chartForm.rows

                        // Επιλογή ημέρας: κλικ, ή Tab → ↑/↓ → Enter
                        activeFocusOnTab: true
                        keyNavigationEnabled: true
                        currentIndex: -1
                        highlightMoveDuration: 0
                        highlight: Rectangle {
                            color: "#0b2a0f"
                            border.color: root.neon
                            border.width: 1
                        }
                        onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
                        onActiveFocusChanged: if (activeFocus && currentIndex < 0 && count > 0) currentIndex = count - 1
                        Keys.onReturnPressed: (event) => { if (currentIndex >= 0) dayForm.openDay(chartForm.rows[currentIndex].date) }
                        Keys.onEnterPressed: (event) => { if (currentIndex >= 0) dayForm.openDay(chartForm.rows[currentIndex].date) }
                        ScrollBar.vertical: ScrollBar {
                            contentItem: Rectangle { implicitWidth: 6; radius: 3; color: root.neonDim }
                            background: Rectangle { color: "transparent" }
                        }

                        delegate: Item {
                            width: ListView.view.width
                            height: 24

                            Rectangle {
                                anchors.fill: parent
                                color: dayClick.containsMouse ? "#0b2a0f" : "transparent"
                            }
                            TermBarRow {
                                anchors.horizontalCenter: parent.horizontalCenter
                                height: parent.height
                                date: modelData.date
                                score: modelData.score
                                blocks: modelData.blocks
                                level: modelData.level
                                barLength: chartForm.barLength
                            }
                            MouseArea {
                                id: dayClick
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    chartList.currentIndex = index
                                    dayForm.openDay(modelData.date)
                                }
                            }
                        }
                    }
                }
            }

            Text {
                id: chartStats
                text: ""
                color: root.cyan
                font.family: root.mono
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
                visible: !chartForm.showActivities
            }
            Text {
                text: "click a day (or Tab, Up/Down, Enter) to open it - you can edit it there"
                color: root.neonDim
                font.family: root.mono
                font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
                visible: !chartForm.showActivities
            }

            // ============ ΣΕΛΙΔΑ 2: ΔΙΑΓΡΑΜΜΑΤΑ ΑΝΑ ΔΡΑΣΤΗΡΙΟΤΗΤΑ ============
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: 6
                visible: chartForm.showActivities

                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(parent.width, 640)
                    color: "#cc050805"
                    border.color: root.neonDark
                    border.width: 1

                    ListView {
                        id: actList
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true
                        model: chartForm.actRows
                        ScrollBar.vertical: ScrollBar {
                            contentItem: Rectangle { implicitWidth: 6; radius: 3; color: root.neonDim }
                            background: Rectangle { color: "transparent" }
                        }

                        delegate: Item {
                            width: ListView.view.width
                            height: modelData.type === "bar" ? 24 : (modelData.type === "title" ? 28 : 18)

                            Loader {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                                sourceComponent: modelData.type === "bar" ? actBarComp : actTextComp
                            }
                            Component {
                                id: actBarComp
                                TermBarRow {
                                    date: modelData.date
                                    score: modelData.score
                                    blocks: modelData.blocks
                                    level: modelData.level
                                    barLength: chartForm.barLength
                                }
                            }
                            Component {
                                id: actTextComp
                                Text {
                                    text: modelData.type === "banner" ? "=".repeat(58)
                                        : (modelData.type === "sep" ? "-".repeat(58) : modelData.text)
                                    color: modelData.type === "title" ? root.neon : root.neonDim
                                    font.family: root.mono
                                    font.pixelSize: modelData.type === "title" ? 16 : 14
                                    font.bold: modelData.type === "title"
                                }
                            }
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: chartForm.actRows.length === 0
                        text: "[INFO] No activity data found yet."
                        color: root.neonDim
                        font.family: root.mono
                        font.pixelSize: 14
                    }
                }
            }

            // Υπόμνημα (αλλάζει ανάλογα με τη σελίδα)
            Row {
                spacing: 18
                Layout.alignment: Qt.AlignHCenter
                Row {
                    spacing: 6
                    Rectangle { width: 10; height: 10; color: root.neon; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: chartForm.showActivities ? ">= 80% of its best day" : "score >= 100 or >= 80% of max"
                        color: root.neonDim; font.family: root.mono; font.pixelSize: 12
                    }
                }
                Row {
                    spacing: 6
                    Rectangle { width: 10; height: 10; color: root.amber; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: ">= 50%"; color: root.neonDim; font.family: root.mono; font.pixelSize: 12 }
                }
                Row {
                    spacing: 6
                    Rectangle { width: 10; height: 10; color: root.warn; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "< 50%"; color: root.neonDim; font.family: root.mono; font.pixelSize: 12 }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 4
                spacing: 20

                HackerButton {
                    text: chartForm.showActivities ? "Total Chart" : "Activity Charts"
                    font.pixelSize: 16
                    onClicked: chartForm.showActivities = !chartForm.showActivities
                }
                HackerButton {
                    text: "Export TXT"
                    font.pixelSize: 16
                    onClicked: {
                        let path = Rate_24_Backend.exportToText()
                        chartStatus.color = path !== "" ? root.neon : root.warn
                        chartStatus.text = path !== "" ? "exported: " + path : "[ERROR] export failed"
                    }
                }
                HackerButton {
                    text: "Back"
                    font.pixelSize: 16
                    visible: chartForm.fromInput
                    onClicked: root.closeBrowse()
                }
                HackerButton {
                    text: "Exit Application"
                    font.pixelSize: 16
                    onClicked: Qt.quit()
                }
            }
            Text {
                id: chartStatus
                text: ""
                color: root.neon
                font.family: root.mono
                font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
                visible: text !== ""
            }

            function loadChart() {
                let data = Rate_24_Backend.getHistoryData(daysFilter.value)

                // Όπως στο CLI: οι μπάρες είναι σχετικές με τη μέγιστη τιμή (ελάχιστο 0.1)
                let maxVal = 0.1
                let sum = 0
                let bestIdx = 0
                for (let i = 0; i < data.length; i++) {
                    let s = data[i].score
                    sum += s
                    if (s > maxVal) maxVal = s
                    if (s > data[bestIdx].score) bestIdx = i
                }

                let built = []
                for (let i = 0; i < data.length; i++) {
                    let pct = data[i].score / maxVal
                    built.push({
                        date: String(data[i].date),
                        score: data[i].score,
                        blocks: Math.max(0, Math.min(barLength, Math.floor(pct * barLength))),
                        level: (data[i].score >= 100 || pct >= 0.8) ? 2 : (pct >= 0.5 ? 1 : 0)
                    })
                }
                rows = built

                // Διαγράμματα ανά δραστηριότητα: κάθε δραστηριότητα έχει τη δική της κλίμακα (όπως στο CLI)
                let acts = Rate_24_Backend.getActivityHistory(daysFilter.value)
                let flat = []
                for (let a = 0; a < acts.length; a++) {
                    let arows = acts[a].rows
                    let m = 0.1
                    for (let r = 0; r < arows.length; r++) {
                        if (arows[r].score > m) m = arows[r].score
                    }
                    flat.push({ type: "banner" })
                    flat.push({ type: "title", text: "EVOLUTION: " + acts[a].name })
                    flat.push({ type: "banner" })
                    for (let r = 0; r < arows.length; r++) {
                        let p = arows[r].score / m
                        flat.push({
                            type: "bar",
                            date: String(arows[r].date),
                            score: arows[r].score,
                            blocks: Math.max(0, Math.min(barLength, Math.floor(p * barLength))),
                            level: p >= 0.8 ? 2 : (p >= 0.5 ? 1 : 0)
                        })
                    }
                    flat.push({ type: "sep" })
                }
                actRows = flat

                chartTitle.text = "TOTAL DAY SCORE EVOLUTION (" +
                                  (daysFilter.value === 0 ? "all " : "Last ") + data.length + " days)"
                chartInfo.text = "[" + data.length + " entries]"
                if (data.length > 0) {
                    chartStats.text = "AVG " + (sum / data.length).toFixed(1) +
                                      "  |  BEST " + data[bestIdx].score.toFixed(1) +
                                      " (" + data[bestIdx].date + ")"
                } else {
                    chartStats.text = "[INFO] No data found yet to display charts."
                }
                console.log("[chart] requested days =", daysFilter.value,
                            "| backend returned", data.length, "entries")
            }
        }

        // --- 5. DAY DETAILS / EDITOR: χρόνος, ποιότητα, SD και σημειώσεις μιας ημέρας (επεξεργάσιμα) ---
        ColumnLayout {
            id: dayForm
            visible: false
            spacing: 10
            Layout.fillWidth: true
            Layout.fillHeight: true

            property var day: ({})
            property bool dirty: false          // υπάρχουν μη αποθηκευμένες αλλαγές
            property bool discardArmed: false   // το πρώτο Esc προειδοποιεί, το δεύτερο απορρίπτει
            property bool saved: false          // έγιναν αλλαγές → το διάγραμμα πρέπει να ξαναφορτωθεί

            function markDirty() {
                dirty = true
                discardArmed = false
                dayStatus.color = root.amber
                dayStatus.text = "* unsaved changes   (Ctrl+S = save)"
            }

            function openDay(date) {
                day = Rate_24_Backend.getDayDetails(date)
                dirty = false
                discardArmed = false
                saved = false
                dayStatus.text = ""
                chartForm.visible = false
                dayForm.visible = true
                Qt.callLater(function() { dayForm.focusCard(0) })
            }

            function closeDay() {
                if (dirty && !discardArmed) {
                    discardArmed = true
                    dayStatus.color = root.warn
                    dayStatus.text = "Unsaved changes! Press Esc again to discard, or Ctrl+S to save."
                    return
                }
                dirty = false
                discardArmed = false
                dayForm.visible = false
                chartForm.visible = true
                if (saved) {
                    saved = false
                    chartForm.loadChart()    // το σκορ άλλαξε → ανανέωση διαγραμμάτων
                }
                Qt.callLater(function() { chartList.forceActiveFocus() })   // επιστροφή στην επιλεγμένη ημέρα
            }

            // Συλλέγει τις τιμές των πεδίων και ζητά από το backend να ξανα-υπολογίσει και να σώσει
            function saveEdits() {
                if (!day.found)
                    return
                let edits = []
                for (let i = 0; i < dayRepeater.count; i++) {
                    let c = dayRepeater.itemAt(i)
                    if (!c) continue
                    edits.push({ real_time: c.editTime, quality: c.editQuality, sd_used: c.editSd, note: c.editNote })
                }
                let d = day.date
                if (Rate_24_Backend.updateDay(d, edits, legacyField.text)) {
                    saved = true
                    dirty = false
                    discardArmed = false
                    day = Rate_24_Backend.getDayDetails(d)   // εμφάνιση των νέων, ξανα-υπολογισμένων σκορ
                    dayStatus.color = root.neon
                    dayStatus.text = "saved - scores recalculated"
                    Qt.callLater(function() { daySaveBtn.forceActiveFocus() })
                } else {
                    dayStatus.color = root.warn
                    dayStatus.text = "[ERROR] could not save this day"
                }
            }

            // Μετακίνηση focus στην επόμενη κάρτα (ή στο Save στο τέλος)
            function focusCard(i) {
                if (i < 0) i = 0
                if (i >= dayRepeater.count) {
                    daySaveBtn.forceActiveFocus()
                } else {
                    let c = dayRepeater.itemAt(i)
                    if (c) c.timeField.forceActiveFocus()
                }
            }

            // Scroll με το πληκτρολόγιο (↑ ↓ PageUp PageDown) όταν δεν γράφεις σε πεδίο
            function scrollBy(dy) {
                let f = dayScroll.contentItem
                f.contentY = Math.max(0, Math.min(f.contentHeight - f.height, f.contentY + dy))
            }
            Keys.onUpPressed: scrollBy(-60)
            Keys.onDownPressed: scrollBy(60)
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_PageUp) { scrollBy(-300); event.accepted = true }
                else if (event.key === Qt.Key_PageDown) { scrollBy(300); event.accepted = true }
            }

            Text {
                text: "> DAY REPORT: " + (dayForm.day.date ? dayForm.day.date : "")
                color: root.neon
                font.family: root.mono
                font.pixelSize: 22
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: dayForm.day.found ? "Final score: " + Number(dayForm.day.score).toFixed(1)
                                        : "[INFO] No data found for this date."
                color: root.cyan
                font.family: root.mono
                font.pixelSize: 16
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                visible: text !== ""
                text: dayForm.day.missed ? "[ MISSED DAY - auto-filled with zeros. Fill it in below and save. ]"
                    : (dayForm.day.incomplete ? "[ INCOMPLETE - the program was closed before the day was saved ]"
                    : (dayForm.day.edited ? "[ EDITED ]" : ""))
                color: dayForm.day.edited ? root.cyan : root.amber
                font.family: root.mono
                font.pixelSize: 13
                Layout.alignment: Qt.AlignHCenter
            }

            ScrollView {
                id: dayScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical: ScrollBar {
                    contentItem: Rectangle { implicitWidth: 6; radius: 3; color: root.neonDim }
                    background: Rectangle { color: "transparent" }
                }

                ColumnLayout {
                    width: dayScroll.availableWidth
                    spacing: 14

                    Repeater {
                        id: dayRepeater
                        model: dayForm.day.activities ? dayForm.day.activities : []

                        delegate: ColumnLayout {
                            id: dcard
                            Layout.fillWidth: true
                            spacing: 0

                            property alias timeField: timeEdit
                            property alias noteField: noteEdit
                            // Τρέχουσες τιμές των πεδίων (με τους ίδιους κανόνες όπως στην κανονική εκτέλεση)
                            readonly property int editTime: timeEdit.text === "" ? 0 : Math.min(1440, parseInt(timeEdit.text))
                            readonly property int editQuality: (editTime <= 0 || qualEdit.text === "") ? 0 : Math.min(100, parseInt(qualEdit.text))
                            // Το SD επιτρέπεται μόνο όταν ο χρόνος είναι λίγος αλλά η ποιότητα καλή
                            readonly property bool sdAllowed: editTime > 0 && editTime < modelData.min_time && editQuality > modelData.min_quality
                            readonly property bool editSd: sdBox.checked && sdAllowed
                            readonly property string editNote: noteEdit.text

                            // Τίτλος δραστηριότητας
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                color: root.neonDark
                                border.color: root.neonDim
                                border.width: 1

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "> " + modelData.name
                                    color: root.neon
                                    font.family: root.mono
                                    font.pixelSize: 16
                                    font.bold: true
                                }
                                Text {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "score: " + Number(modelData.score).toFixed(1)
                                    color: root.cyan
                                    font.family: root.mono
                                    font.pixelSize: 14
                                }
                            }

                            // Χρόνος / ποιότητα / SD
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 46
                                color: root.panel
                                border.color: root.neonDark
                                border.width: 1

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.leftMargin: 10
                                    spacing: 10

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "time:"
                                        color: root.neonDim
                                        font.family: root.mono
                                        font.pixelSize: 14
                                    }
                                    HackerField {
                                        id: timeEdit
                                        anchors.verticalCenter: parent.verticalCenter
                                        implicitWidth: 80
                                        font.pixelSize: 14
                                        maximumLength: 4
                                        text: String(modelData.real_time)
                                        validator: RegularExpressionValidator { regularExpression: /^[0-9]+$/ }
                                        onTextEdited: dayForm.markDirty()
                                        onAccepted: qualEdit.enabled ? qualEdit.forceActiveFocus() : noteEdit.forceActiveFocus()
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "/ " + modelData.min_time + " min"
                                        color: dcard.editTime >= modelData.min_time ? root.neon : root.warn
                                        font.family: root.mono
                                        font.pixelSize: 14
                                    }

                                    Item { width: 14; height: 1 }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "quality:"
                                        color: root.neonDim
                                        font.family: root.mono
                                        font.pixelSize: 14
                                    }
                                    HackerField {
                                        id: qualEdit
                                        anchors.verticalCenter: parent.verticalCenter
                                        implicitWidth: 80
                                        font.pixelSize: 14
                                        maximumLength: 3
                                        enabled: dcard.editTime > 0          // χωρίς χρόνο η ποιότητα δεν μετράει
                                        text: String(modelData.quality)
                                        validator: IntValidator { bottom: 0; top: 100 }
                                        onTextEdited: dayForm.markDirty()
                                        onAccepted: sdBox.enabled ? sdBox.forceActiveFocus() : noteEdit.forceActiveFocus()
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: dcard.editTime <= 0 ? "(ignored)" : "/ " + modelData.min_quality
                                        color: dcard.editTime <= 0 ? root.neonDim
                                             : (dcard.editQuality >= modelData.min_quality ? root.neon : root.warn)
                                        font.family: root.mono
                                        font.pixelSize: 14
                                    }

                                    Item { width: 14; height: 1 }

                                    CheckBox {
                                        id: sdBox
                                        anchors.verticalCenter: parent.verticalCenter
                                        checked: modelData.sd_used
                                        enabled: dcard.sdAllowed
                                        text: "SD"
                                        font.family: root.mono
                                        font.pixelSize: 14
                                        onToggled: dayForm.markDirty()
                                        indicator: Rectangle {
                                            x: sdBox.leftPadding
                                            y: parent.height / 2 - height / 2
                                            implicitWidth: 18
                                            implicitHeight: 18
                                            color: "transparent"
                                            border.color: sdBox.activeFocus ? "#ffffff" : (sdBox.enabled ? root.amber : root.neonDark)
                                            border.width: sdBox.activeFocus ? 2 : 1
                                            Text {
                                                anchors.centerIn: parent
                                                text: (sdBox.checked && sdBox.enabled) ? "x" : ""
                                                color: root.amber
                                                font.family: root.mono
                                                font.pixelSize: 14
                                                font.bold: true
                                            }
                                        }
                                        contentItem: Text {
                                            text: sdBox.text
                                            color: sdBox.enabled ? root.amber : root.neonDim
                                            font: sdBox.font
                                            leftPadding: sdBox.indicator.width + 8
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        Keys.onReturnPressed: (event) => { if (!event.isAutoRepeat) { toggle(); dayForm.markDirty() } }
                                    }
                                }
                            }

                            // Σημειώσεις της δραστηριότητας (επεξεργάσιμες)
                            TextArea {
                                id: noteEdit
                                Layout.fillWidth: true
                                Layout.preferredHeight: Math.max(64, implicitHeight)
                                visible: dayForm.day.has_activity_notes
                                text: modelData.note
                                color: root.neon
                                font.family: root.mono
                                font.pixelSize: 15
                                wrapMode: TextArea.WrapAtWordBoundaryOrAnywhere   // σπάει και μακριές λέξεις/χαρακτήρες χωρίς κενά
                                placeholderText: "// notes..."
                                placeholderTextColor: root.neonDim
                                selectionColor: root.neon
                                selectedTextColor: "#000000"
                                leftPadding: 10
                                topPadding: 8
                                onTextChanged: if (activeFocus) dayForm.markDirty()
                                onCursorRectangleChanged: if (activeFocus) root.ensureCursorVisible(dayScroll, noteEdit)
                                onActiveFocusChanged: if (activeFocus) root.ensureCursorVisible(dayScroll, noteEdit)
                                Keys.onTabPressed: (event) => { event.accepted = true; dayForm.focusCard(index + 1) }
                                Keys.onBacktabPressed: (event) => {
                                    event.accepted = true
                                    if (qualEdit.enabled) qualEdit.forceActiveFocus(); else timeEdit.forceActiveFocus()
                                }
                                background: Rectangle {
                                    color: root.panel
                                    border.color: noteEdit.activeFocus ? root.neon : root.neonDark
                                    border.width: noteEdit.activeFocus ? 2 : 1
                                }
                            }
                        }
                    }

                    // Παλιές εγγραφές (πριν σωθούν σημειώσεις ανά δραστηριότητα): ένα κοινό, επεξεργάσιμο κείμενο
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        visible: dayForm.day.found && !dayForm.day.has_activity_notes

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            color: root.neonDark
                            border.color: root.neonDim
                            border.width: 1
                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: "> NOTES (older entry: all activities together)"
                                color: root.neon
                                font.family: root.mono
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }
                        TextArea {
                            id: legacyField
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.max(80, implicitHeight)
                            text: dayForm.day.notes ? dayForm.day.notes : ""
                            color: root.neon
                            font.family: root.mono
                            font.pixelSize: 15
                            wrapMode: TextArea.WrapAtWordBoundaryOrAnywhere   // σπάει και μακριές λέξεις/χαρακτήρες χωρίς κενά
                            placeholderText: "// notes..."
                            placeholderTextColor: root.neonDim
                            selectionColor: root.neon
                            selectedTextColor: "#000000"
                            leftPadding: 10
                            topPadding: 8
                            onTextChanged: if (activeFocus) dayForm.markDirty()
                            onCursorRectangleChanged: if (activeFocus) root.ensureCursorVisible(dayScroll, legacyField)
                            onActiveFocusChanged: if (activeFocus) root.ensureCursorVisible(dayScroll, legacyField)
                            Keys.onTabPressed: (event) => { event.accepted = true; daySaveBtn.forceActiveFocus() }
                            background: Rectangle {
                                color: root.panel
                                border.color: legacyField.activeFocus ? root.neon : root.neonDark
                                border.width: legacyField.activeFocus ? 2 : 1
                            }
                        }
                    }
                }
            }

            Text {
                id: dayStatus
                text: ""
                color: root.neon
                font.family: root.mono
                font.pixelSize: 13
                Layout.alignment: Qt.AlignHCenter
                visible: text !== ""
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                HackerButton {
                    id: daySaveBtn
                    text: "Save Changes"
                    font.pixelSize: 18
                    enabled: dayForm.day.found
                    onClicked: dayForm.saveEdits()
                }
                HackerButton {
                    id: dayBackBtn
                    text: "Back to Chart"
                    font.pixelSize: 18
                    onClicked: dayForm.closeDay()
                }
            }
            Text {
                text: "[Ctrl+S = save  |  Esc = back  |  Tab = next field  |  SD only if time < min and quality > min]"
                color: root.neonDim
                font.family: root.mono
                font.pixelSize: 12
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
