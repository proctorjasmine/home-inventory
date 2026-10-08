let allInventory = [];
let allUsers = [];
let allLocations = [];
let allProducts = [];

let selectedItem = null;
let selectedUser = null;
let consumeQuantity = 1;


/* =========================================================
   INITIALIZE
   ========================================================= */

async function initializeApp() {
    await Promise.all([
        loadUsers(),
        loadLocations(),
        loadProducts()
    ]);

    await loadInventory();
}


/* =========================================================
   USERS / THEME
   ========================================================= */

async function loadUsers() {
    const response = await fetch("/api/users");

    if (!response.ok) {
        throw new Error("Could not load users.");
    }

    allUsers = await response.json();

    const storedId =
        Number(localStorage.getItem("inventoryUserId"));

    selectedUser =
        allUsers.find(user => user.id === storedId) ||
        null;

    if (selectedUser) {
        applySelectedUser();
    }
    else {
        updateProfileButton();
    }
}


function applySelectedUser() {
    localStorage.setItem(
        "inventoryUserId",
        selectedUser.id
    );

    localStorage.setItem(
        "inventoryUserName",
        selectedUser.name
    );

    document.body.classList.toggle(
        "theme-jasmine",
        selectedUser.name.toLowerCase() === "jasmine"
    );

    updateProfileButton();
}


function updateProfileButton() {
    const button =
        document.getElementById("profile-button");

    button.textContent =
        selectedUser
            ? selectedUser.name.charAt(0).toUpperCase()
            : "?";

    button.title =
        selectedUser
            ? selectedUser.name
            : "Select user";
}


function renderUserOptions() {
    const container =
        document.getElementById("user-options");

    container.innerHTML = "";

    for (const user of allUsers) {
        const button =
            document.createElement("button");

        button.className =
            "user-option" +
            (
                selectedUser &&
                selectedUser.id === user.id
                    ? " selected"
                    : ""
            );

        button.innerHTML = `
            <span class="user-avatar">
                ${escapeHtml(
                    user.name.charAt(0).toUpperCase()
                )}
            </span>

            <span class="user-name">
                ${escapeHtml(user.name)}
            </span>

            ${
                selectedUser &&
                selectedUser.id === user.id
                    ? '<span class="user-current">Current</span>'
                    : ""
            }
        `;

        button.addEventListener("click", () => {
            selectedUser = user;
            applySelectedUser();
            closeUserSheet();
        });

        container.appendChild(button);
    }
}


function openUserSheet() {
    renderUserOptions();

    document
        .getElementById("user-overlay")
        .classList.add("open");

    document
        .getElementById("user-sheet")
        .classList.add("open");
}


function closeUserSheet() {
    document
        .getElementById("user-overlay")
        .classList.remove("open");

    document
        .getElementById("user-sheet")
        .classList.remove("open");
}


/* =========================================================
   DATA
   ========================================================= */

async function loadLocations() {
    const response = await fetch("/api/locations");

    if (!response.ok) {
        throw new Error("Could not load locations.");
    }

    allLocations = await response.json();
}


async function loadProducts() {
    const response = await fetch("/api/products");

    if (!response.ok) {
        throw new Error("Could not load products.");
    }

    allProducts = await response.json();
    renderLowStock();
}


async function loadInventory() {
    const response = await fetch("/api/inventory");

    if (!response.ok) {
        throw new Error("Could not load inventory.");
    }

    allInventory = await response.json();
    applySearch();
}


async function refreshInventoryData() {
    await Promise.all([
        loadProducts(),
        loadInventory()
    ]);
}


/* =========================================================
   LOW STOCK
   ========================================================= */

function renderLowStock() {
    const section =
        document.getElementById("low-stock-section");

    const container =
        document.getElementById("low-stock");

    const count =
        document.getElementById("low-stock-count");

    const lowProducts =
        allProducts.filter(
            product => product.is_low_stock
        );

    section.hidden = lowProducts.length === 0;

    if (lowProducts.length === 0) {
        container.innerHTML = "";
        return;
    }

    count.textContent =
        `${lowProducts.length} low`;

    container.innerHTML = "";

    for (const product of lowProducts) {
        const card =
            document.createElement("button");

        card.className = "low-stock-card";

        card.innerHTML = `
            <div>
                <strong>
                    ${escapeHtml(product.name)}
                </strong>

                <span>
                    ${product.total_quantity}
                    ${escapeHtml(
                        pluralize(
                            product.unit,
                            product.total_quantity
                        )
                    )}
                    total
                </span>
            </div>

            <span class="low-stock-threshold">
                Low at ${product.low_stock_threshold}
            </span>
        `;

        card.addEventListener("click", () => {
            const item =
                allInventory.find(
                    row =>
                        row.product_id === product.id
                );

            if (item) {
                openManageSheet(item);
            }
        });

        container.appendChild(card);
    }
}


/* =========================================================
   INVENTORY
   ========================================================= */

function renderInventory(items) {
    const container =
        document.getElementById("inventory");

    const itemCount =
        document.getElementById("item-count");

    container.innerHTML = "";

    itemCount.textContent =
        `${items.length} ${
            items.length === 1 ? "item" : "items"
        }`;

    if (items.length === 0) {
        container.innerHTML =
            '<p class="loading">No inventory found.</p>';
        return;
    }

    const grouped = {};

    for (const item of items) {
        if (!grouped[item.location]) {
            grouped[item.location] = [];
        }

        grouped[item.location].push(item);
    }

    for (
        const [location, locationItems]
        of Object.entries(grouped)
    ) {
        const card = document.createElement("div");
        card.className = "location-card";

        const header = document.createElement("div");
        header.className = "location-header";

        const title = document.createElement("h3");
        title.textContent = location;

        const count = document.createElement("span");
        count.textContent =
            `${locationItems.length} ${
                locationItems.length === 1
                    ? "item"
                    : "items"
            }`;

        header.append(title, count);
        card.appendChild(header);

        for (const item of locationItems) {
            const row = document.createElement("div");
            row.className = "inventory-item";

            row.addEventListener(
                "click",
                () => openManageSheet(item)
            );

            row.innerHTML = `
                <div>
                    <p class="item-name">
                        ${escapeHtml(item.product)}
                    </p>

                    <p class="item-location">
                        ${escapeHtml(item.location)}
                    </p>
                </div>

                <div class="item-quantity">
                    <span class="number">
                        ${item.quantity}
                    </span>

                    <span class="unit">
                        ${escapeHtml(
                            pluralize(
                                item.unit,
                                item.quantity
                            )
                        )}
                    </span>
                </div>
            `;

            card.appendChild(row);
        }

        container.appendChild(card);
    }
}


/* =========================================================
   MANAGEMENT SHEET
   ========================================================= */

function openManageSheet(item) {
    selectedItem = item;
    consumeQuantity = 1;

    document
        .getElementById("sheet-product")
        .textContent = item.product;

    document
        .getElementById("sheet-location")
        .textContent = item.location;

    document
        .getElementById("sheet-stock-number")
        .textContent = item.quantity;

    document
        .getElementById("sheet-stock-unit")
        .textContent =
            pluralize(item.unit, item.quantity);

    document
        .getElementById("manage-error")
        .textContent = "";

    hideManagementPanels();
    populateMoveLocations();
    populateEditForm();
    updateConsumeControls();

    document
        .getElementById("sheet-overlay")
        .classList.add("open");

    document
        .getElementById("manage-sheet")
        .classList.add("open");
}


function closeManageSheet() {
    document
        .getElementById("sheet-overlay")
        .classList.remove("open");

    document
        .getElementById("manage-sheet")
        .classList.remove("open");

    selectedItem = null;
}


function hideManagementPanels() {
    for (
        const panel of
        document.querySelectorAll(".management-panel")
    ) {
        panel.hidden = true;
    }
}


function showManagementPanel(panelId) {
    hideManagementPanels();

    document
        .getElementById(panelId)
        .hidden = false;

    document
        .getElementById("manage-error")
        .textContent = "";
}


function requireUser() {
    if (selectedUser) {
        return true;
    }

    closeManageSheet();
    openUserSheet();
    return false;
}


/* =========================================================
   CONSUME
   ========================================================= */

function updateConsumeControls() {
    document
        .getElementById("consume-quantity")
        .textContent = consumeQuantity;

    const unit =
        selectedItem
            ? pluralize(
                selectedItem.unit,
                consumeQuantity
            )
            : "items";

    document
        .getElementById("consume-button")
        .textContent =
            `Consume ${consumeQuantity} ${unit}`;
}


async function consumeInventory() {
    if (!selectedItem || !requireUser()) {
        return;
    }

    await runManagementRequest(
        "consume-button",
        "/api/inventory/remove",
        {
            product_id: selectedItem.product_id,
            location_id: selectedItem.location_id,
            user_id: selectedUser.id,
            quantity: consumeQuantity
        },
        "Consuming..."
    );
}


/* =========================================================
   PHYSICAL COUNT
   ========================================================= */

async function savePhysicalCount() {
    if (!selectedItem || !requireUser()) {
        return;
    }

    const input =
        document.getElementById("count-quantity");

    const newQuantity = Number(input.value);

    if (
        input.value.trim() === "" ||
        !Number.isInteger(newQuantity) ||
        newQuantity < 0
    ) {
        showManageError(
            "Enter a whole number of 0 or more."
        );
        return;
    }

    await runManagementRequest(
        "save-count-button",
        "/api/inventory/adjust",
        {
            product_id: selectedItem.product_id,
            location_id: selectedItem.location_id,
            user_id: selectedUser.id,
            new_quantity: newQuantity
        },
        "Saving..."
    );
}


/* =========================================================
   MOVE
   ========================================================= */

function populateMoveLocations() {
    const select =
        document.getElementById("move-location");

    select.innerHTML = "";

    for (const location of allLocations) {
        if (
            location.id === selectedItem.location_id
        ) {
            continue;
        }

        const option =
            document.createElement("option");

        option.value = location.id;
        option.textContent = location.name;

        select.appendChild(option);
    }

    document
        .getElementById("move-quantity")
        .value = "1";
}


async function moveInventory() {
    if (!selectedItem || !requireUser()) {
        return;
    }

    const destinationId =
        Number(
            document
                .getElementById("move-location")
                .value
        );

    const quantityInput =
        document.getElementById("move-quantity");

    const quantity =
        Number(quantityInput.value);

    if (!destinationId) {
        showManageError(
            "Choose a destination location."
        );
        return;
    }

    if (
        quantityInput.value.trim() === "" ||
        !Number.isInteger(quantity) ||
        quantity <= 0
    ) {
        showManageError(
            "Enter a positive whole-number quantity."
        );
        return;
    }

    if (quantity > selectedItem.quantity) {
        showManageError(
            `Only ${selectedItem.quantity} in this location.`
        );
        return;
    }

    await runManagementRequest(
        "move-button",
        "/api/inventory/move",
        {
            product_id: selectedItem.product_id,
            source_location_id:
                selectedItem.location_id,
            destination_location_id:
                destinationId,
            user_id: selectedUser.id,
            quantity
        },
        "Moving..."
    );
}


/* =========================================================
   EDIT PRODUCT
   ========================================================= */

function getSelectedProduct() {
    if (!selectedItem) {
        return null;
    }

    return allProducts.find(
        product =>
            product.id === selectedItem.product_id
    ) || null;
}


function populateEditForm() {
    const product = getSelectedProduct();

    if (!product) {
        return;
    }

    document.getElementById("edit-name").value =
        product.name;

    document.getElementById("edit-brand").value =
        product.brand || "";

    document.getElementById("edit-unit").value =
        product.unit;

    document.getElementById("edit-threshold").value =
        product.low_stock_threshold;

    document
        .getElementById("edit-auto-grocery")
        .checked = product.auto_add_grocery;
}


async function saveProduct() {
    const product = getSelectedProduct();

    if (!product) {
        return;
    }

    const name =
        document
            .getElementById("edit-name")
            .value
            .trim();

    const brand =
        document
            .getElementById("edit-brand")
            .value
            .trim();

    const unit =
        document
            .getElementById("edit-unit")
            .value
            .trim();

    const thresholdInput =
        document.getElementById("edit-threshold");

    const threshold =
        Number(thresholdInput.value);

    if (!name || !unit) {
        showManageError(
            "Product name and inventory unit are required."
        );
        return;
    }

    if (
        thresholdInput.value.trim() === "" ||
        !Number.isInteger(threshold) ||
        threshold < 0
    ) {
        showManageError(
            "Low-stock threshold must be 0 or more."
        );
        return;
    }

    const button =
        document.getElementById(
            "save-product-button"
        );

    button.disabled = true;
    button.textContent = "Saving...";
    showManageError("");

    try {
        const response =
            await fetch(
                `/api/products/${product.id}`,
                {
                    method: "PATCH",
                    headers: {
                        "Content-Type":
                            "application/json"
                    },
                    body: JSON.stringify({
                        name,
                        brand,
                        inventory_unit: unit,
                        low_stock_threshold:
                            threshold,
                        auto_add_grocery:
                            document
                                .getElementById(
                                    "edit-auto-grocery"
                                )
                                .checked
                    })
                }
            );

        const result = await response.json();

        if (!response.ok) {
            throw new Error(
                result.error ||
                "Could not update product."
            );
        }

        closeManageSheet();
        await refreshInventoryData();
    }
    catch (error) {
        showManageError(error.message);
    }
    finally {
        button.disabled = false;
        button.textContent = "Save Product";
    }
}


/* =========================================================
   REQUEST HELPER
   ========================================================= */

async function runManagementRequest(
    buttonId,
    url,
    body,
    busyText
) {
    const button =
        document.getElementById(buttonId);

    const originalText =
        button.textContent;

    button.disabled = true;
    button.textContent = busyText;
    showManageError("");

    try {
        const response =
            await fetch(
                url,
                {
                    method: "POST",
                    headers: {
                        "Content-Type":
                            "application/json"
                    },
                    body: JSON.stringify(body)
                }
            );

        const result = await response.json();

        if (!response.ok) {
            throw new Error(
                result.error ||
                "Could not update inventory."
            );
        }

        closeManageSheet();
        await refreshInventoryData();
    }
    catch (error) {
        showManageError(error.message);
    }
    finally {
        button.disabled = false;
        button.textContent = originalText;

        if (buttonId === "consume-button") {
            updateConsumeControls();
        }
    }
}


function showManageError(message) {
    document
        .getElementById("manage-error")
        .textContent = message;
}


/* =========================================================
   SEARCH / TEXT
   ========================================================= */

function applySearch() {
    const search =
        document
            .getElementById("search-input")
            .value
            .trim()
            .toLowerCase();

    const filtered =
        allInventory.filter(
            item =>
                item.product
                    .toLowerCase()
                    .includes(search) ||
                item.location
                    .toLowerCase()
                    .includes(search)
        );

    renderInventory(filtered);
}


function pluralize(unit, quantity) {
    if (quantity === 1) {
        return unit;
    }

    return `${unit}s`;
}


function escapeHtml(value) {
    const element =
        document.createElement("div");

    element.textContent = value ?? "";
    return element.innerHTML;
}


/* =========================================================
   EVENTS
   ========================================================= */

document
    .getElementById("search-input")
    .addEventListener("input", applySearch);

document
    .getElementById("profile-button")
    .addEventListener("click", openUserSheet);

document
    .getElementById("user-sheet-close")
    .addEventListener("click", closeUserSheet);

document
    .getElementById("user-overlay")
    .addEventListener("click", closeUserSheet);

document
    .getElementById("sheet-close")
    .addEventListener("click", closeManageSheet);

document
    .getElementById("sheet-overlay")
    .addEventListener("click", closeManageSheet);

document
    .getElementById("show-consume")
    .addEventListener(
        "click",
        () => showManagementPanel("consume-panel")
    );

document
    .getElementById("show-count")
    .addEventListener(
        "click",
        () => {
            document
                .getElementById("count-quantity")
                .value = selectedItem.quantity;

            showManagementPanel("count-panel");
        }
    );

document
    .getElementById("show-move")
    .addEventListener(
        "click",
        () => showManagementPanel("move-panel")
    );

document
    .getElementById("show-edit")
    .addEventListener(
        "click",
        () => showManagementPanel("edit-panel")
    );

document
    .getElementById("quantity-minus")
    .addEventListener(
        "click",
        () => {
            if (consumeQuantity > 1) {
                consumeQuantity--;
                updateConsumeControls();
            }
        }
    );

document
    .getElementById("quantity-plus")
    .addEventListener(
        "click",
        () => {
            if (
                selectedItem &&
                consumeQuantity <
                    selectedItem.quantity
            ) {
                consumeQuantity++;
                updateConsumeControls();
            }
        }
    );

document
    .getElementById("consume-button")
    .addEventListener("click", consumeInventory);

document
    .getElementById("save-count-button")
    .addEventListener("click", savePhysicalCount);

document
    .getElementById("move-button")
    .addEventListener("click", moveInventory);

document
    .getElementById("save-product-button")
    .addEventListener("click", saveProduct);


function openScanner() {
    window.location.href = "/scan";
}

document
    .getElementById("scan-button")
    .addEventListener("click", openScanner);

document
    .getElementById("nav-scan-button")
    .addEventListener("click", openScanner);

document
    .getElementById("nav-history-button")
    .addEventListener(
        "click",
        () => {
            window.location.href = "/history";
        }
    );


/* =========================================================
   START
   ========================================================= */

initializeApp()
    .catch(error => {
        console.error(error);

        document
            .getElementById("inventory")
            .innerHTML =
                '<p class="loading">' +
                'Could not load inventory.' +
                '</p>';
    });
