let selectedPackage = null;
let selectedUser = null;
let allLocations = [];
let allProducts = [];
let pendingBarcode = null;

let codeReader = null;
let scannerControls = null;
let barcodeHandled = false;


/* =========================================================
   INITIALIZE
   ========================================================= */

async function initializeScanner() {
    await loadCurrentUser();
    await loadLocations();
    await loadProducts();
    initializeZXing();
}


/* =========================================================
   CURRENT USER + THEME
   ========================================================= */

async function loadCurrentUser() {
    const response = await fetch("/api/users");

    if (!response.ok) {
        throw new Error("Could not load users.");
    }

    const users = await response.json();

    const storedUserId =
        localStorage.getItem("inventoryUserId");

    if (storedUserId !== null) {
        selectedUser =
            users.find(
                user =>
                    user.id === Number(storedUserId)
            ) || null;
    }

    if (
        selectedUser &&
        selectedUser.name.toLowerCase() === "jasmine"
    ) {
        document.body.classList.add(
            "theme-jasmine"
        );
    }
}


/* =========================================================
   LOCATIONS
   ========================================================= */

async function loadLocations() {
    const response =
        await fetch("/api/locations");

    if (!response.ok) {
        throw new Error(
            "Could not load locations."
        );
    }

    allLocations =
        await response.json();

    const select =
        document.getElementById(
            "restock-location"
        );

    select.innerHTML = "";

    const storageLocations =
        allLocations.filter(location => {

            const hasChildren =
                allLocations.some(
                    other =>
                        other.parent_id ===
                        location.id
                );

            return !hasChildren;
        });

    for (const location of storageLocations) {
        const option =
            document.createElement("option");

        option.value = location.id;
        option.textContent = location.name;

        select.appendChild(option);
    }
}


/* =========================================================
   PRODUCTS
   ========================================================= */

async function loadProducts() {
    const response =
        await fetch("/api/products");

    if (!response.ok) {
        throw new Error(
            "Could not load products."
        );
    }

    allProducts = await response.json();

    populateProductSelect();
}


function populateProductSelect() {
    const select =
        document.getElementById(
            "existing-product-select"
        );

    select.innerHTML = "";

    for (const product of allProducts) {
        const option =
            document.createElement("option");

        option.value = product.id;

        option.textContent =
            product.brand
                ? `${product.name} — ${product.brand}`
                : product.name;

        option.dataset.unit =
            product.unit;

        select.appendChild(option);
    }

    updateExistingPackageUnit();
}


function updateExistingPackageUnit() {
    const select =
        document.getElementById(
            "existing-product-select"
        );

    const quantity =
        Number(
            document.getElementById(
                "existing-package-quantity"
            ).value
        ) || 1;

    const option =
        select.options[
            select.selectedIndex
        ];

    const unit =
        option
            ? option.dataset.unit
            : "item";

    document.getElementById(
        "existing-package-unit"
    ).textContent =
        pluralize(unit, quantity);
}


/* =========================================================
   ZXING
   ========================================================= */

function initializeZXing() {
    if (
        typeof ZXingBrowser === "undefined"
    ) {
        throw new Error(
            "ZXing barcode library did not load."
        );
    }

    codeReader =
        new ZXingBrowser
            .BrowserMultiFormatOneDReader();

    console.log(
        "ZXing barcode scanner ready."
    );
}


/* =========================================================
   CAMERA + LIVE SCANNING
   ========================================================= */

async function startCamera() {
    const button =
        document.getElementById(
            "start-camera-button"
        );

    const video =
        document.getElementById(
            "camera-preview"
        );

    const error =
        document.getElementById(
            "scan-error"
        );

    error.textContent = "";
    barcodeHandled = false;

    if (!codeReader) {
        error.textContent =
            "Barcode scanner is not ready.";

        return;
    }

    try {
        button.disabled = true;
        button.textContent =
            "Starting camera...";

        scannerControls =
            await codeReader.decodeFromConstraints(
                {
                    video: {
                        facingMode: {
                            ideal: "environment"
                        },

                        width: {
                            ideal: 1920
                        },

                        height: {
                            ideal: 1080
                        }
                    },

                    audio: false
                },

                video,

                async (
                    result,
                    scanError,
                    controls
                ) => {

                    if (
                        result &&
                        !barcodeHandled
                    ) {
                        barcodeHandled = true;

                        const barcode =
                            result.getText();

                        console.log(
                            "Barcode detected:",
                            barcode
                        );

                        document
                            .getElementById(
                                "barcode-input"
                            )
                            .value =
                                barcode;

                        controls.stop();

                        scannerControls = null;

                        button.hidden = false;
                        button.disabled = false;
                        button.textContent =
                            "Scan Another Barcode";

                        await lookupBarcode(
                            barcode
                        );
                    }
                }
            );

        button.hidden = true;
    }
    catch (err) {
        console.error(err);

        button.hidden = false;
        button.disabled = false;
        button.textContent =
            "Start Camera";

        error.textContent =
            "Could not start barcode scanner.";
    }
}


function stopScanner() {
    if (scannerControls) {
        scannerControls.stop();
        scannerControls = null;
    }
}


/* =========================================================
   MANUAL BARCODE ENTRY
   ========================================================= */

document
    .getElementById("barcode-form")
    .addEventListener(
        "submit",
        async event => {

            event.preventDefault();

            const input =
                document.getElementById(
                    "barcode-input"
                );

            const barcode =
                input.value.trim();

            if (!barcode) {
                document
                    .getElementById(
                        "scan-error"
                    )
                    .textContent =
                        "Enter a barcode.";

                return;
            }

            stopScanner();

            await lookupBarcode(barcode);
        }
    );


/* =========================================================
   BARCODE LOOKUP
   ========================================================= */

async function lookupBarcode(barcode) {
    const error =
        document.getElementById(
            "scan-error"
        );

    const resultSection =
        document.getElementById(
            "scan-result"
        );

    const newProductSection =
        document.getElementById(
            "new-product-section"
        );

    const unknownSection =
        document.getElementById(
            "unknown-barcode-section"
        );

    const existingPackageSection =
        document.getElementById(
            "existing-package-section"
        );

    error.textContent = "";

    resultSection.hidden = true;
    newProductSection.hidden = true;
    unknownSection.hidden = true;
    existingPackageSection.hidden = true;

    selectedPackage = null;

    try {
        const response =
            await fetch(
                `/api/products/barcode/${
                    encodeURIComponent(barcode)
                }`
            );

        /*
         * Unknown barcode is not an error.
         * It means the user needs to tell us
         * what this barcode represents.
         */
        if (response.status === 404) {
            pendingBarcode = barcode;

            document.getElementById(
                "new-product-barcode"
            ).value = barcode;

            document.getElementById(
                "existing-package-barcode"
            ).value = barcode;

            unknownSection.hidden = false;

            unknownSection.scrollIntoView({
                behavior: "smooth",
                block: "start"
            });

            return;
        }

        const product =
            await response.json();

        if (!response.ok) {
            throw new Error(
                product.error ||
                "Could not look up barcode."
            );
        }

        pendingBarcode = null;
        selectedPackage = product;

        showProduct(product);
    }
    catch (err) {
        console.error(err);

        error.textContent =
            err.message;
    }
}


/* =========================================================
   UNKNOWN BARCODE CHOICE
   ========================================================= */

document
    .getElementById(
        "existing-product-choice"
    )
    .addEventListener(
        "click",
        () => {

            document.getElementById(
                "unknown-barcode-section"
            ).hidden = true;

            document.getElementById(
                "new-product-section"
            ).hidden = true;

            const section =
                document.getElementById(
                    "existing-package-section"
                );

            section.hidden = false;

            document.getElementById(
                "existing-package-barcode"
            ).value =
                pendingBarcode || "";

            updateExistingPackageUnit();

            section.scrollIntoView({
                behavior: "smooth",
                block: "start"
            });
        }
    );


document
    .getElementById(
        "new-product-choice"
    )
    .addEventListener(
        "click",
        () => {

            document.getElementById(
                "unknown-barcode-section"
            ).hidden = true;

            document.getElementById(
                "existing-package-section"
            ).hidden = true;

            const section =
                document.getElementById(
                    "new-product-section"
                );

            section.hidden = false;

            document.getElementById(
                "new-product-barcode"
            ).value =
                pendingBarcode || "";

            section.scrollIntoView({
                behavior: "smooth",
                block: "start"
            });
        }
    );


/* =========================================================
   EXISTING PRODUCT PACKAGE
   ========================================================= */

document
    .getElementById(
        "existing-product-select"
    )
    .addEventListener(
        "change",
        updateExistingPackageUnit
    );


document
    .getElementById(
        "existing-package-quantity"
    )
    .addEventListener(
        "input",
        updateExistingPackageUnit
    );


document
    .getElementById(
        "back-to-barcode-choice"
    )
    .addEventListener(
        "click",
        () => {

            document.getElementById(
                "existing-package-section"
            ).hidden = true;

            document.getElementById(
                "unknown-barcode-section"
            ).hidden = false;
        }
    );


document
    .getElementById(
        "existing-package-form"
    )
    .addEventListener(
        "submit",
        async event => {

            event.preventDefault();

            const error =
                document.getElementById(
                    "scan-error"
                );

            const button =
                document.getElementById(
                    "save-package-button"
                );

            const productId =
                Number(
                    document.getElementById(
                        "existing-product-select"
                    ).value
                );

            const barcode =
                document.getElementById(
                    "existing-package-barcode"
                ).value.trim();

            const packageQuantity =
                Number(
                    document.getElementById(
                        "existing-package-quantity"
                    ).value
                );

            if (!productId) {
                error.textContent =
                    "Choose an existing product.";

                return;
            }

            if (
                !Number.isInteger(packageQuantity) ||
                packageQuantity <= 0
            ) {
                error.textContent =
                    "Package quantity must be a positive whole number.";

                return;
            }

            error.textContent = "";

            button.disabled = true;
            button.textContent =
                "Saving Package...";

            try {
                const response =
                    await fetch(
                        "/api/product-packages",
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body:
                                JSON.stringify({
                                    product_id:
                                        productId,

                                    barcode:
                                        barcode,

                                    package_quantity:
                                        packageQuantity
                                })
                        }
                    );

                const result =
                    await response.json();

                if (!response.ok) {
                    throw new Error(
                        result.error ||
                        "Could not add package."
                    );
                }

                /*
                 * The barcode now exists in the
                 * database. Run the normal lookup
                 * again so we land on the same
                 * restock screen as any known barcode.
                 */
                await lookupBarcode(barcode);

                document.getElementById(
                    "scan-result"
                ).scrollIntoView({
                    behavior: "smooth",
                    block: "start"
                });
            }
            catch (err) {
                console.error(err);

                error.textContent =
                    err.message;
            }
            finally {
                button.disabled = false;
                button.textContent =
                    "Save Package";
            }
        }
    );


/* =========================================================
   NEW PRODUCT
   ========================================================= */

document
    .getElementById(
        "back-to-new-barcode-choice"
    )
    .addEventListener(
        "click",
        () => {

            document.getElementById(
                "new-product-section"
            ).hidden = true;

            document.getElementById(
                "unknown-barcode-section"
            ).hidden = false;
        }
    );


document
    .getElementById(
        "new-product-form"
    )
    .addEventListener(
        "submit",
        async event => {

            event.preventDefault();

            const error =
                document.getElementById(
                    "scan-error"
                );

            const button =
                document.getElementById(
                    "create-product-button"
                );

            const barcode =
                document
                    .getElementById(
                        "new-product-barcode"
                    )
                    .value
                    .trim();

            const name =
                document
                    .getElementById(
                        "new-product-name"
                    )
                    .value
                    .trim();

            const brand =
                document
                    .getElementById(
                        "new-product-brand"
                    )
                    .value
                    .trim();

            const packageQuantity =
                Number(
                    document
                        .getElementById(
                            "new-product-quantity"
                        )
                        .value
                );

            const inventoryUnit =
                document
                    .getElementById(
                        "new-product-unit"
                    )
                    .value;

            const lowStockThreshold =
                Number(
                    document
                        .getElementById(
                            "new-product-threshold"
                        )
                        .value
                );

            const autoAddGrocery =
                document
                    .getElementById(
                        "new-product-grocery"
                    )
                    .checked;

            error.textContent = "";

            button.disabled = true;
            button.textContent =
                "Creating...";

            try {
                const response =
                    await fetch(
                        "/api/products",
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body:
                                JSON.stringify({
                                    name:
                                        name,

                                    brand:
                                        brand,

                                    inventory_unit:
                                        inventoryUnit,

                                    package_quantity:
                                        packageQuantity,

                                    low_stock_threshold:
                                        lowStockThreshold,

                                    auto_add_grocery:
                                        autoAddGrocery,

                                    barcode:
                                        barcode
                                })
                        }
                    );

                const result =
                    await response.json();

                if (!response.ok) {
                    throw new Error(
                        result.error ||
                        "Could not create product."
                    );
                }

                await loadProducts();

                /*
                 * Product now exists.
                 * Reuse the normal lookup path.
                 */
                await lookupBarcode(barcode);

                document
                    .getElementById(
                        "scan-result"
                    )
                    .scrollIntoView({
                        behavior: "smooth",
                        block: "start"
                    });
            }
            catch (err) {
                console.error(err);

                error.textContent =
                    err.message;
            }
            finally {
                button.disabled = false;
                button.textContent =
                    "Create Product";
            }
        }
    );


/* =========================================================
   SHOW FOUND PRODUCT
   ========================================================= */

function showProduct(product) {
    document
        .getElementById(
            "scan-product"
        )
        .textContent =
            product.product;

    document
        .getElementById(
            "scan-brand"
        )
        .textContent =
            product.brand || "";

    document
        .getElementById(
            "scan-package-quantity"
        )
        .textContent =
            product.package_quantity;

    document
        .getElementById(
            "scan-package-unit"
        )
        .textContent =
            pluralize(
                product.unit,
                product.package_quantity
            );

    document
        .getElementById(
            "restock-button"
        )
        .textContent =
            `Add ${
                product.package_quantity
            } ${
                pluralize(
                    product.unit,
                    product.package_quantity
                )
            }`;

    document
        .getElementById(
            "scan-result"
        )
        .hidden = false;
}


/* =========================================================
   RESTOCK
   ========================================================= */

document
    .getElementById(
        "restock-button"
    )
    .addEventListener(
        "click",
        async () => {

            const error =
                document.getElementById(
                    "scan-error"
                );

            if (!selectedPackage) {
                return;
            }

            if (!selectedUser) {
                error.textContent =
                    "Select a user from the " +
                    "home screen first.";

                return;
            }

            const locationId =
                Number(
                    document
                        .getElementById(
                            "restock-location"
                        )
                        .value
                );

            const button =
                document.getElementById(
                    "restock-button"
                );

            button.disabled = true;
            button.textContent =
                "Adding...";

            error.textContent = "";

            try {
                const response =
                    await fetch(
                        "/api/inventory/add",
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body:
                                JSON.stringify({
                                    barcode:
                                        selectedPackage
                                            .barcode,

                                    location_id:
                                        locationId,

                                    user_id:
                                        selectedUser.id
                                })
                        }
                    );

                const result =
                    await response.json();

                if (!response.ok) {
                    throw new Error(
                        result.error ||
                        "Could not add inventory."
                    );
                }

                window.location.href = "/";
            }
            catch (err) {
                error.textContent =
                    err.message;

                button.disabled = false;

                button.textContent =
                    `Add ${
                        selectedPackage
                            .package_quantity
                    } ${
                        pluralize(
                            selectedPackage.unit,
                            selectedPackage
                                .package_quantity
                        )
                    }`;
            }
        }
    );


/* =========================================================
   CAMERA BUTTON
   ========================================================= */

document
    .getElementById(
        "start-camera-button"
    )
    .addEventListener(
        "click",
        startCamera
    );


/* =========================================================
   HELPERS
   ========================================================= */

function pluralize(unit, quantity) {
    if (quantity === 1) {
        return unit;
    }

    return `${unit}s`;
}


/* =========================================================
   CLEANUP
   ========================================================= */

window.addEventListener(
    "pagehide",
    stopScanner
);


/* =========================================================
   START APP
   ========================================================= */

initializeScanner()
    .catch(error => {

        console.error(error);

        document
            .getElementById(
                "scan-error"
            )
            .textContent =
                "Could not initialize scanner.";
    });