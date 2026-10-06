let allHistory = [];


/* =========================================================
   INITIALIZE
   ========================================================= */

async function initializeHistory() {
    loadCurrentUser();
    await loadHistory();
}


/* =========================================================
   CURRENT USER / THEME
   ========================================================= */

function loadCurrentUser() {
    const button =
        document.getElementById("profile-button");

    const storedUserId =
        localStorage.getItem("inventoryUserId");

    const storedUserName =
        localStorage.getItem("inventoryUserName");

    /*
     * app.js currently stores the user ID.
     * If no name has been stored yet, we fetch users below.
     */
    if (!storedUserId) {
        button.textContent = "?";
        return;
    }

    loadUserFromApi(Number(storedUserId));
}


async function loadUserFromApi(userId) {
    try {
        const response =
            await fetch("/api/users");

        if (!response.ok) {
            return;
        }

        const users =
            await response.json();

        const user =
            users.find(
                item => item.id === userId
            );

        if (!user) {
            return;
        }

        const button =
            document.getElementById(
                "profile-button"
            );

        button.textContent =
            user.name
                .charAt(0)
                .toUpperCase();

        button.title = user.name;

        if (
            user.name.toLowerCase()
            === "jasmine"
        ) {
            document.body.classList.add(
                "theme-jasmine"
            );
        }
    }
    catch (error) {
        console.error(error);
    }
}


/* =========================================================
   LOAD HISTORY
   ========================================================= */

async function loadHistory() {
    const response =
        await fetch("/api/history");

    if (!response.ok) {
        throw new Error(
            "Could not load history."
        );
    }

    allHistory =
        await response.json();

    renderHistory(allHistory);
}


/* =========================================================
   RENDER
   ========================================================= */

function renderHistory(transactions) {
    const container =
        document.getElementById("history");

    const count =
        document.getElementById(
            "history-count"
        );

    container.innerHTML = "";

    count.textContent =
        `${transactions.length} ${
            transactions.length === 1
                ? "change"
                : "changes"
        }`;

    if (transactions.length === 0) {
        container.innerHTML = `
            <p class="loading">
                No inventory history yet.
            </p>
        `;

        return;
    }

    const grouped = new Map();

    for (const transaction of transactions) {
        const date =
            parseUtcTimestamp(
                transaction.created_at
            );

        const key =
            getDateGroupKey(date);

        if (!grouped.has(key)) {
            grouped.set(key, []);
        }

        grouped
            .get(key)
            .push({
                ...transaction,
                localDate: date
            });
    }

    for (
        const [dateLabel, entries]
        of grouped
    ) {
        const group =
            document.createElement("section");

        group.className =
            "history-day";

        const heading =
            document.createElement("h3");

        heading.className =
            "history-day-heading";

        heading.textContent =
            dateLabel;

        group.appendChild(heading);

        const card =
            document.createElement("div");

        card.className =
            "history-card";

        for (const entry of entries) {
            card.appendChild(
                createHistoryRow(entry)
            );
        }

        group.appendChild(card);
        container.appendChild(group);
    }
}


function createHistoryRow(transaction) {
    const row =
        document.createElement("div");

    row.className = "history-item";

    const isRestock =
        transaction.quantity_change > 0;

    const quantity =
        Math.abs(
            transaction.quantity_change
        );

    const action =
        isRestock
            ? "Restocked"
            : "Used";

    const icon =
        isRestock
            ? "+"
            : "−";

    const unit =
        pluralize(
            transaction.unit,
            quantity
        );

    const time =
        transaction.localDate
            .toLocaleTimeString(
                [],
                {
                    hour: "numeric",
                    minute: "2-digit"
                }
            );

    row.innerHTML = `
        <div
            class="history-icon
            ${isRestock
                ? "history-add"
                : "history-remove"}"
        >
            ${icon}
        </div>

        <div class="history-details">

            <div class="history-title-row">
                <p class="history-product">
                    ${escapeHtml(
                        transaction.product
                    )}
                </p>

                <span class="history-quantity">
                    ${isRestock ? "+" : "−"}${quantity}
                </span>
            </div>

            <p class="history-action">
                ${action}
                ${quantity}
                ${escapeHtml(unit)}
                ·
                ${escapeHtml(
                    transaction.location
                )}
            </p>

            <p class="history-meta">
                ${escapeHtml(
                    transaction.user
                )}
                ·
                ${time}
            </p>

        </div>
    `;

    return row;
}


/* =========================================================
   DATES
   ========================================================= */

function parseUtcTimestamp(timestamp) {
    /*
     * SQLite CURRENT_TIMESTAMP returns:
     *
     * 2026-10-06 01:24:31
     *
     * That value is UTC, but it has no timezone
     * marker. Convert the space to T and append Z
     * so JavaScript knows it is UTC.
     */
    return new Date(
        timestamp.replace(" ", "T") + "Z"
    );
}


function getDateGroupKey(date) {
    const now = new Date();

    const today =
        startOfDay(now);

    const transactionDay =
        startOfDay(date);

    const difference =
        Math.round(
            (
                today -
                transactionDay
            ) /
            86400000
        );

    if (difference === 0) {
        return "Today";
    }

    if (difference === 1) {
        return "Yesterday";
    }

    return date.toLocaleDateString(
        [],
        {
            month: "long",
            day: "numeric",
            year:
                date.getFullYear() !==
                now.getFullYear()
                    ? "numeric"
                    : undefined
        }
    );
}


function startOfDay(date) {
    return new Date(
        date.getFullYear(),
        date.getMonth(),
        date.getDate()
    );
}


/* =========================================================
   TEXT HELPERS
   ========================================================= */

function pluralize(unit, quantity) {
    if (quantity === 1) {
        return unit;
    }

    return `${unit}s`;
}


function escapeHtml(value) {
    const element =
        document.createElement("div");

    element.textContent =
        value ?? "";

    return element.innerHTML;
}


/* =========================================================
   NAVIGATION
   ========================================================= */

document
    .getElementById("nav-home-button")
    .addEventListener(
        "click",
        () => {
            window.location.href = "/";
        }
    );


document
    .getElementById("nav-scan-button")
    .addEventListener(
        "click",
        () => {
            window.location.href = "/scan";
        }
    );


/* =========================================================
   START
   ========================================================= */

initializeHistory()
    .catch(error => {

        console.error(error);

        document
            .getElementById("history")
            .innerHTML = `
                <p class="loading">
                    Could not load history.
                </p>
            `;
    });