(window["webpackJsonp"] = window["webpackJsonp"] || []).push([["main"],{

/***/ "./src/$$_lazy_route_resource lazy recursive":
/*!**********************************************************!*\
  !*** ./src/$$_lazy_route_resource lazy namespace object ***!
  \**********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

function webpackEmptyAsyncContext(req) {
	// Here Promise.resolve().then() is used instead of new Promise() to prevent
	// uncaught exception popping up in devtools
	return Promise.resolve().then(function() {
		var e = new Error("Cannot find module '" + req + "'");
		e.code = 'MODULE_NOT_FOUND';
		throw e;
	});
}
webpackEmptyAsyncContext.keys = function() { return []; };
webpackEmptyAsyncContext.resolve = webpackEmptyAsyncContext;
module.exports = webpackEmptyAsyncContext;
webpackEmptyAsyncContext.id = "./src/$$_lazy_route_resource lazy recursive";

/***/ }),

/***/ "./src/app/_helpers/directives/input-validate/input-validate.directive.ts":
/*!********************************************************************************!*\
  !*** ./src/app/_helpers/directives/input-validate/input-validate.directive.ts ***!
  \********************************************************************************/
/*! exports provided: InputValidateDirective */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "InputValidateDirective", function() { return InputValidateDirective; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var inputmask__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! inputmask */ "./node_modules/inputmask/index.js");
/* harmony import */ var inputmask__WEBPACK_IMPORTED_MODULE_1___default = /*#__PURE__*/__webpack_require__.n(inputmask__WEBPACK_IMPORTED_MODULE_1__);
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};


var InputValidateDirective = /** @class */ (function () {
    function InputValidateDirective(el) {
        this.el = el;
        this.regexMap = {
            integer: {
                regex: '^[0-9]*$',
                placeholder: ''
            },
            // float: '^[+-]?([0-9]*[.])?[0-9]+$',
            // words: '([A-z]*\\s)*',
            // point25: '^\-?[0-9]*(?:\\.25|\\.50|\\.75|)$',
            money: {
                alias: 'decimal',
                digits: 8,
                max: 999999999999,
                rightAlign: false,
                allowMinus: false,
                onBeforeMask: function (value) { return value; }
            }
        };
    }
    Object.defineProperty(InputValidateDirective.prototype, "defineInputType", {
        set: function (type) {
            inputmask__WEBPACK_IMPORTED_MODULE_1__(this.regexMap[type]).mask(this.el.nativeElement);
        },
        enumerable: true,
        configurable: true
    });
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])('appInputValidate'),
        __metadata("design:type", String),
        __metadata("design:paramtypes", [String])
    ], InputValidateDirective.prototype, "defineInputType", null);
    InputValidateDirective = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Directive"])({
            selector: '[appInputValidate]'
        }),
        __metadata("design:paramtypes", [_angular_core__WEBPACK_IMPORTED_MODULE_0__["ElementRef"]])
    ], InputValidateDirective);
    return InputValidateDirective;
}());



/***/ }),

/***/ "./src/app/_helpers/directives/modal-container/modal-container.component.html":
/*!************************************************************************************!*\
  !*** ./src/app/_helpers/directives/modal-container/modal-container.component.html ***!
  \************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"modal\">\r\n  <div class=\"content\">\r\n    <i class=\"icon\"></i>\r\n    <div class=\"message\">\r\n      <span>Success</span>\r\n      <span>Successfully</span>\r\n    </div>\r\n  </div>\r\n  <button type=\"button\" class=\"action-button\" (click)=\"false\">OK</button>\r\n  <button type=\"button\" class=\"close-button\" (click)=\"false\"><i class=\"icon close\"></i></button>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/_helpers/directives/modal-container/modal-container.component.scss":
/*!************************************************************************************!*\
  !*** ./src/app/_helpers/directives/modal-container/modal-container.component.scss ***!
  \************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: fixed;\n  top: 0;\n  bottom: 0;\n  left: 0;\n  right: 0;\n  display: flex;\n  align-items: center;\n  justify-content: center;\n  background: rgba(255, 255, 255, 0.25); }\n\n.modal {\n  position: relative;\n  display: flex;\n  flex-direction: column;\n  padding: 2rem;\n  width: 34rem;\n  background: #1a1a1a; }\n\n.modal .content {\n    display: flex;\n    margin: 1.2rem 0; }\n\n.modal .content .icon {\n      width: 4.4rem;\n      height: 4.4rem; }\n\n.modal .content .message {\n      display: flex;\n      flex-direction: column;\n      margin-left: 2rem; }\n\n.modal .action-button {\n    background: #2c95f1;\n    margin: 1.2rem auto 0.6rem;\n    width: 10rem;\n    height: 2.4rem; }\n\n.modal .close-button {\n    position: absolute;\n    top: 0.5rem;\n    right: 0.5rem;\n    display: flex;\n    align-items: center;\n    justify-content: center;\n    background: transparent;\n    margin: 0;\n    padding: 0;\n    width: 1.5rem;\n    height: 1.5rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvX2hlbHBlcnMvZGlyZWN0aXZlcy9tb2RhbC1jb250YWluZXIvRDpcXHphbm8vc3JjXFxhcHBcXF9oZWxwZXJzXFxkaXJlY3RpdmVzXFxtb2RhbC1jb250YWluZXJcXG1vZGFsLWNvbnRhaW5lci5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGdCQUFlO0VBQ2YsT0FBTTtFQUNOLFVBQVM7RUFDVCxRQUFPO0VBQ1AsU0FBUTtFQUNSLGNBQWE7RUFDYixvQkFBbUI7RUFDbkIsd0JBQXVCO0VBQ3ZCLHNDQUFxQyxFQUN0Qzs7QUFDRDtFQUNFLG1CQUFrQjtFQUNsQixjQUFhO0VBQ2IsdUJBQXNCO0VBQ3RCLGNBQWE7RUFDYixhQUFZO0VBQ1osb0JBQW1CLEVBc0NwQjs7QUE1Q0Q7SUFTSSxjQUFhO0lBQ2IsaUJBQWdCLEVBWWpCOztBQXRCSDtNQWFNLGNBQWE7TUFDYixlQUFjLEVBQ2Y7O0FBZkw7TUFrQk0sY0FBYTtNQUNiLHVCQUFzQjtNQUN0QixrQkFBaUIsRUFDbEI7O0FBckJMO0lBeUJJLG9CQUFtQjtJQUNuQiwyQkFBMEI7SUFDMUIsYUFBWTtJQUNaLGVBQWMsRUFDZjs7QUE3Qkg7SUFnQ0ksbUJBQWtCO0lBQ2xCLFlBQVc7SUFDWCxjQUFhO0lBQ2IsY0FBYTtJQUNiLG9CQUFtQjtJQUNuQix3QkFBdUI7SUFDdkIsd0JBQXVCO0lBQ3ZCLFVBQVM7SUFDVCxXQUFVO0lBQ1YsY0FBYTtJQUNiLGVBQWMsRUFDZiIsImZpbGUiOiJzcmMvYXBwL19oZWxwZXJzL2RpcmVjdGl2ZXMvbW9kYWwtY29udGFpbmVyL21vZGFsLWNvbnRhaW5lci5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBwb3NpdGlvbjogZml4ZWQ7XHJcbiAgdG9wOiAwO1xyXG4gIGJvdHRvbTogMDtcclxuICBsZWZ0OiAwO1xyXG4gIHJpZ2h0OiAwO1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICBiYWNrZ3JvdW5kOiByZ2JhKDI1NSwgMjU1LCAyNTUsIDAuMjUpO1xyXG59XHJcbi5tb2RhbCB7XHJcbiAgcG9zaXRpb246IHJlbGF0aXZlO1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICBwYWRkaW5nOiAycmVtO1xyXG4gIHdpZHRoOiAzNHJlbTtcclxuICBiYWNrZ3JvdW5kOiAjMWExYTFhO1xyXG5cclxuICAuY29udGVudCB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgbWFyZ2luOiAxLjJyZW0gMDtcclxuXHJcbiAgICAuaWNvbiB7XHJcbiAgICAgIHdpZHRoOiA0LjRyZW07XHJcbiAgICAgIGhlaWdodDogNC40cmVtO1xyXG4gICAgfVxyXG5cclxuICAgIC5tZXNzYWdlIHtcclxuICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgICAgbWFyZ2luLWxlZnQ6IDJyZW07XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAuYWN0aW9uLWJ1dHRvbiB7XHJcbiAgICBiYWNrZ3JvdW5kOiAjMmM5NWYxO1xyXG4gICAgbWFyZ2luOiAxLjJyZW0gYXV0byAwLjZyZW07XHJcbiAgICB3aWR0aDogMTByZW07XHJcbiAgICBoZWlnaHQ6IDIuNHJlbTtcclxuICB9XHJcblxyXG4gIC5jbG9zZS1idXR0b24ge1xyXG4gICAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gICAgdG9wOiAwLjVyZW07XHJcbiAgICByaWdodDogMC41cmVtO1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgIGJhY2tncm91bmQ6IHRyYW5zcGFyZW50O1xyXG4gICAgbWFyZ2luOiAwO1xyXG4gICAgcGFkZGluZzogMDtcclxuICAgIHdpZHRoOiAxLjVyZW07XHJcbiAgICBoZWlnaHQ6IDEuNXJlbTtcclxuICB9XHJcbn1cclxuIl19 */"

/***/ }),

/***/ "./src/app/_helpers/directives/modal-container/modal-container.component.ts":
/*!**********************************************************************************!*\
  !*** ./src/app/_helpers/directives/modal-container/modal-container.component.ts ***!
  \**********************************************************************************/
/*! exports provided: ModalContainerComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ModalContainerComponent", function() { return ModalContainerComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};

var ModalContainerComponent = /** @class */ (function () {
    function ModalContainerComponent() {
    }
    ModalContainerComponent.prototype.ngOnInit = function () {
    };
    ModalContainerComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-modal-container',
            template: __webpack_require__(/*! ./modal-container.component.html */ "./src/app/_helpers/directives/modal-container/modal-container.component.html"),
            styles: [__webpack_require__(/*! ./modal-container.component.scss */ "./src/app/_helpers/directives/modal-container/modal-container.component.scss")]
        }),
        __metadata("design:paramtypes", [])
    ], ModalContainerComponent);
    return ModalContainerComponent;
}());



/***/ }),

/***/ "./src/app/_helpers/directives/staking-switch/staking-switch.component.html":
/*!**********************************************************************************!*\
  !*** ./src/app/_helpers/directives/staking-switch/staking-switch.component.html ***!
  \**********************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"switch\" (click)=\"toggleStaking()\">\r\n  <span class=\"option\" *ngIf=\"staking\">ON</span>\r\n  <span class=\"circle\" [class.on]=\"staking\" [class.off]=\"!staking\"></span>\r\n  <span class=\"option\" *ngIf=\"!staking\">OFF</span>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/_helpers/directives/staking-switch/staking-switch.component.scss":
/*!**********************************************************************************!*\
  !*** ./src/app/_helpers/directives/staking-switch/staking-switch.component.scss ***!
  \**********************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".switch {\n  display: flex;\n  align-items: center;\n  justify-content: space-between;\n  border-radius: 1rem;\n  cursor: pointer;\n  font-size: 1rem;\n  padding: 0.5rem;\n  width: 5rem;\n  height: 2rem; }\n  .switch .circle {\n    border-radius: 1rem;\n    width: 1.2rem;\n    height: 1.2rem; }\n  .switch .option {\n    margin: 0 0.2rem;\n    line-height: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvX2hlbHBlcnMvZGlyZWN0aXZlcy9zdGFraW5nLXN3aXRjaC9EOlxcemFuby9zcmNcXGFwcFxcX2hlbHBlcnNcXGRpcmVjdGl2ZXNcXHN0YWtpbmctc3dpdGNoXFxzdGFraW5nLXN3aXRjaC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGNBQWE7RUFDYixvQkFBbUI7RUFDbkIsK0JBQThCO0VBQzlCLG9CQUFtQjtFQUNuQixnQkFBZTtFQUNmLGdCQUFlO0VBQ2YsZ0JBQWU7RUFDZixZQUFXO0VBQ1gsYUFBWSxFQVliO0VBckJEO0lBWUksb0JBQW1CO0lBQ25CLGNBQWE7SUFDYixlQUFjLEVBQ2Y7RUFmSDtJQWtCSSxpQkFBZ0I7SUFDaEIsb0JBQW1CLEVBQ3BCIiwiZmlsZSI6InNyYy9hcHAvX2hlbHBlcnMvZGlyZWN0aXZlcy9zdGFraW5nLXN3aXRjaC9zdGFraW5nLXN3aXRjaC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIi5zd2l0Y2gge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgYm9yZGVyLXJhZGl1czogMXJlbTtcclxuICBjdXJzb3I6IHBvaW50ZXI7XHJcbiAgZm9udC1zaXplOiAxcmVtO1xyXG4gIHBhZGRpbmc6IDAuNXJlbTtcclxuICB3aWR0aDogNXJlbTtcclxuICBoZWlnaHQ6IDJyZW07XHJcblxyXG4gIC5jaXJjbGUge1xyXG4gICAgYm9yZGVyLXJhZGl1czogMXJlbTtcclxuICAgIHdpZHRoOiAxLjJyZW07XHJcbiAgICBoZWlnaHQ6IDEuMnJlbTtcclxuICB9XHJcblxyXG4gIC5vcHRpb24ge1xyXG4gICAgbWFyZ2luOiAwIDAuMnJlbTtcclxuICAgIGxpbmUtaGVpZ2h0OiAxLjJyZW07XHJcbiAgfVxyXG59XHJcbiJdfQ== */"

/***/ }),

/***/ "./src/app/_helpers/directives/staking-switch/staking-switch.component.ts":
/*!********************************************************************************!*\
  !*** ./src/app/_helpers/directives/staking-switch/staking-switch.component.ts ***!
  \********************************************************************************/
/*! exports provided: StakingSwitchComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "StakingSwitchComponent", function() { return StakingSwitchComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _services_backend_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../../services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _services_variables_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../../services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};



var StakingSwitchComponent = /** @class */ (function () {
    function StakingSwitchComponent(backend, variablesService) {
        this.backend = backend;
        this.variablesService = variablesService;
        this.stakingChange = new _angular_core__WEBPACK_IMPORTED_MODULE_0__["EventEmitter"]();
    }
    StakingSwitchComponent.prototype.ngOnInit = function () {
    };
    StakingSwitchComponent.prototype.toggleStaking = function () {
        var wallet = this.variablesService.getWallet(this.wallet_id);
        if (wallet && wallet.loaded) {
            this.stakingChange.emit(!this.staking);
            if (!this.staking) {
                this.backend.startPosMining(this.wallet_id);
            }
            else {
                this.backend.stopPosMining(this.wallet_id);
            }
        }
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", Boolean)
    ], StakingSwitchComponent.prototype, "wallet_id", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", Boolean)
    ], StakingSwitchComponent.prototype, "staking", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Output"])(),
        __metadata("design:type", Object)
    ], StakingSwitchComponent.prototype, "stakingChange", void 0);
    StakingSwitchComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-staking-switch',
            template: __webpack_require__(/*! ./staking-switch.component.html */ "./src/app/_helpers/directives/staking-switch/staking-switch.component.html"),
            styles: [__webpack_require__(/*! ./staking-switch.component.scss */ "./src/app/_helpers/directives/staking-switch/staking-switch.component.scss")]
        }),
        __metadata("design:paramtypes", [_services_backend_service__WEBPACK_IMPORTED_MODULE_1__["BackendService"], _services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"]])
    ], StakingSwitchComponent);
    return StakingSwitchComponent;
}());



/***/ }),

/***/ "./src/app/_helpers/directives/tooltip.directive.ts":
/*!**********************************************************!*\
  !*** ./src/app/_helpers/directives/tooltip.directive.ts ***!
  \**********************************************************/
/*! exports provided: TooltipDirective */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "TooltipDirective", function() { return TooltipDirective; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};

var TooltipDirective = /** @class */ (function () {
    function TooltipDirective(el, renderer) {
        this.el = el;
        this.renderer = renderer;
        this.offset = 10;
    }
    TooltipDirective.prototype.onMouseEnter = function () {
        if (!this.tooltip) {
            this.show();
        }
    };
    TooltipDirective.prototype.onMouseLeave = function () {
        if (this.tooltip) {
            this.hide();
        }
    };
    TooltipDirective.prototype.show = function () {
        this.create();
        this.setPosition();
    };
    TooltipDirective.prototype.hide = function () {
        var _this = this;
        this.renderer.setStyle(this.tooltip, 'opacity', '0');
        window.setTimeout(function () {
            _this.renderer.removeChild(document.body, _this.tooltip);
            _this.tooltip = null;
        }, this.delay);
    };
    TooltipDirective.prototype.create = function () {
        var _this = this;
        this.tooltip = this.renderer.createElement('span');
        this.renderer.appendChild(this.tooltip, this.renderer.createText(this.tooltipTitle));
        this.renderer.appendChild(document.body, this.tooltip);
        this.renderer.setStyle(document.body, 'position', 'relative');
        this.renderer.setStyle(this.tooltip, 'position', 'absolute');
        if (this.tooltipClass !== null) {
            this.renderer.addClass(this.tooltip, this.tooltipClass);
        }
        if (this.placement !== null) {
            this.renderer.addClass(this.tooltip, 'ng-tooltip-' + this.placement);
        }
        else {
            this.placement = 'top';
            this.renderer.addClass(this.tooltip, 'ng-tooltip-top');
        }
        if (this.delay !== null) {
            this.renderer.setStyle(this.tooltip, 'opacity', '0');
            this.renderer.setStyle(this.tooltip, '-webkit-transition', "opacity " + this.delay + "ms");
            this.renderer.setStyle(this.tooltip, '-moz-transition', "opacity " + this.delay + "ms");
            this.renderer.setStyle(this.tooltip, '-o-transition', "opacity " + this.delay + "ms");
            this.renderer.setStyle(this.tooltip, 'transition', "opacity " + this.delay + "ms");
            window.setTimeout(function () {
                _this.renderer.setStyle(_this.tooltip, 'opacity', '1');
            }, 0);
        }
    };
    TooltipDirective.prototype.setPosition = function () {
        var hostPos = this.el.nativeElement.getBoundingClientRect();
        var tooltipPos = this.tooltip.getBoundingClientRect();
        // const scrollPos = window.pageYOffset || document.documentElement.scrollTop || document.body.scrollTop || 0;
        if (this.placement === 'top') {
            this.renderer.setStyle(this.tooltip, 'top', hostPos.top - tooltipPos.height + 'px');
            this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
        }
        if (this.placement === 'bottom') {
            this.renderer.setStyle(this.tooltip, 'top', hostPos.bottom + 'px');
            this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
        }
        if (this.placement === 'left') {
            this.renderer.setStyle(this.tooltip, 'top', hostPos.top + 'px');
            this.renderer.setStyle(this.tooltip, 'left', hostPos.left - tooltipPos.width + 'px');
        }
        if (this.placement === 'right') {
            this.renderer.setStyle(this.tooltip, 'top', hostPos.top + 'px');
            this.renderer.setStyle(this.tooltip, 'left', hostPos.right + 'px');
        }
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])('tooltip'),
        __metadata("design:type", String)
    ], TooltipDirective.prototype, "tooltipTitle", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", String)
    ], TooltipDirective.prototype, "placement", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", String)
    ], TooltipDirective.prototype, "tooltipClass", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", Number)
    ], TooltipDirective.prototype, "delay", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["HostListener"])('mouseenter'),
        __metadata("design:type", Function),
        __metadata("design:paramtypes", []),
        __metadata("design:returntype", void 0)
    ], TooltipDirective.prototype, "onMouseEnter", null);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["HostListener"])('mouseleave'),
        __metadata("design:type", Function),
        __metadata("design:paramtypes", []),
        __metadata("design:returntype", void 0)
    ], TooltipDirective.prototype, "onMouseLeave", null);
    TooltipDirective = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Directive"])({
            selector: '[tooltip]',
            host: {
                '[style.cursor]': '"pointer"'
            }
        }),
        __metadata("design:paramtypes", [_angular_core__WEBPACK_IMPORTED_MODULE_0__["ElementRef"], _angular_core__WEBPACK_IMPORTED_MODULE_0__["Renderer2"]])
    ], TooltipDirective);
    return TooltipDirective;
}());



/***/ }),

/***/ "./src/app/_helpers/models/wallet.model.ts":
/*!*************************************************!*\
  !*** ./src/app/_helpers/models/wallet.model.ts ***!
  \*************************************************/
/*! exports provided: Wallet */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "Wallet", function() { return Wallet; });
var Wallet = /** @class */ (function () {
    function Wallet(id, name, pass, path, address, balance, unlocked_balance, mined, tracking) {
        if (balance === void 0) { balance = 0; }
        if (unlocked_balance === void 0) { unlocked_balance = 0; }
        if (mined === void 0) { mined = 0; }
        if (tracking === void 0) { tracking = ''; }
        this.contracts = [];
        this.wallet_id = id;
        this.name = name;
        this.pass = pass;
        this.path = path;
        this.address = address;
        this.balance = balance;
        this.unlocked_balance = unlocked_balance;
        this.mined_total = mined;
        this.tracking_hey = tracking;
        this.alias = '';
        this.staking = false;
        this.new_messages = 0;
        this.new_contracts = 0;
        this.history = [];
        this.excluded_history = [];
        this.progress = 0;
        this.loaded = false;
    }
    Wallet.prototype.getMoneyEquivalent = function (equivalent) {
        return this.balance * equivalent;
    };
    Wallet.prototype.havePass = function () {
        return (this.pass !== '' && this.pass !== null);
    };
    Wallet.prototype.isActive = function (id) {
        return this.wallet_id === id;
    };
    Wallet.prototype.prepareHistoryItem = function (item) {
        if (item.tx_type === 4) {
            item.sortFee = -(item.amount + item.fee);
            item.sortAmount = 0;
        }
        else if (item.tx_type === 3) {
            item.sortFee = 0;
        }
        else if ((item.hasOwnProperty('contract') && (item.contract[0].state === 3 || item.contract[0].state === 6 || item.contract[0].state === 601) && !item.contract[0].is_a)) {
            item.sortFee = -item.fee;
            item.sortAmount = item.amount;
        }
        else {
            if (!item.is_income) {
                item.sortFee = -item.fee;
                item.sortAmount = -item.amount;
            }
            else {
                item.sortAmount = item.amount;
            }
        }
        return item;
    };
    Wallet.prototype.prepareHistory = function (items) {
        for (var i = 0; i < items.length; i++) {
            if ((items[i].tx_type === 7 && items[i].is_income) || (items[i].tx_type === 11 && items[i].is_income) || (items[i].amount === 0 && items[i].fee === 0)) {
                var exists = false;
                for (var j = 0; j < this.excluded_history.length; j++) {
                    if (this.excluded_history[j].tx_hash === items[i].tx_hash) {
                        exists = true;
                        if (this.excluded_history[j].height !== items[i].height) {
                            this.excluded_history[j] = items[i];
                        }
                        break;
                    }
                }
                if (!exists) {
                    this.excluded_history.push(items[i]);
                }
            }
            else {
                var exists = false;
                for (var j = 0; j < this.history.length; j++) {
                    if (this.history[j].tx_hash === items[i].tx_hash) {
                        exists = true;
                        if (this.history[j].height !== items[i].height) {
                            this.history[j] = this.prepareHistoryItem(items[i]);
                        }
                        break;
                    }
                }
                if (!exists) {
                    if (this.history.length && items[i].timestamp > this.history[0].timestamp) {
                        this.history.unshift(this.prepareHistoryItem(items[i]));
                    }
                    else {
                        this.history.push(this.prepareHistoryItem(items[i]));
                    }
                }
            }
        }
    };
    Wallet.prototype.prepareContractsAfterOpen = function (items, exp_med_ts, height_app, viewedContracts, notViewedContracts) {
        var safe = this;
        var _loop_1 = function (i) {
            var contract = items[i];
            var contractTransactionExist = false;
            if (safe && safe.history) {
                contractTransactionExist = safe.history.some(function (elem) { return elem.contract && elem.contract.length && elem.contract[0].contract_id === contract.contract_id; });
            }
            if (!contractTransactionExist && safe && safe.excluded_history) {
                contractTransactionExist = safe.excluded_history.some(function (elem) { return elem.contract && elem.contract.length && elem.contract[0].contract_id === contract.contract_id; });
            }
            if (!contractTransactionExist) {
                contract.state = 140;
            }
            else if (contract.state === 1 && contract.expiration_time < exp_med_ts) {
                contract.state = 110;
            }
            else if (contract.state === 2 && contract.cancel_expiration_time !== 0 && contract.cancel_expiration_time < exp_med_ts && contract.height === 0) {
                var searchResult1 = viewedContracts.some(function (elem) { return elem.state === 2 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id; });
                if (!searchResult1) {
                    contract.state = 130;
                    contract.is_new = true;
                }
            }
            else if (contract.state === 1) {
                var searchResult2 = notViewedContracts.find(function (elem) { return elem.state === 110 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id; });
                if (searchResult2) {
                    if (searchResult2.time === contract.expiration_time) {
                        contract.state = 110;
                    }
                    else {
                        for (var j = 0; j < notViewedContracts.length; j++) {
                            if (notViewedContracts[j].contract_id === contract.contract_id && notViewedContracts[j].is_a === contract.is_a) {
                                notViewedContracts.splice(j, 1);
                                break;
                            }
                        }
                        for (var j = 0; j < viewedContracts.length; j++) {
                            if (viewedContracts[j].contract_id === contract.contract_id && viewedContracts[j].is_a === contract.is_a) {
                                viewedContracts.splice(j, 1);
                                break;
                            }
                        }
                    }
                }
            }
            else if (contract.state === 2 && (contract.height === 0 || (height_app - contract.height) < 10)) {
                contract.state = 201;
            }
            else if (contract.state === 2) {
                var searchResult3 = viewedContracts.some(function (elem) { return elem.state === 120 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id; });
                if (searchResult3) {
                    contract.state = 120;
                }
            }
            else if (contract.state === 5) {
                var searchResult4 = notViewedContracts.find(function (elem) { return elem.state === 130 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id; });
                if (searchResult4) {
                    if (searchResult4.time === contract.cancel_expiration_time) {
                        contract.state = 130;
                    }
                    else {
                        for (var j = 0; j < notViewedContracts.length; j++) {
                            if (notViewedContracts[j].contract_id === contract.contract_id && notViewedContracts[j].is_a === contract.is_a) {
                                notViewedContracts.splice(j, 1);
                                break;
                            }
                        }
                        for (var j = 0; j < viewedContracts.length; j++) {
                            if (viewedContracts[j].contract_id === contract.contract_id && viewedContracts[j].is_a === contract.is_a) {
                                viewedContracts.splice(j, 1);
                                break;
                            }
                        }
                    }
                }
            }
            else if (contract.state === 6 && (contract.height === 0 || (height_app - contract.height) < 10)) {
                contract.state = 601;
            }
            var searchResult = viewedContracts.some(function (elem) { return elem.state === contract.state && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id; });
            contract.is_new = !searchResult;
            contract['private_detailes'].a_pledge += contract['private_detailes'].to_pay;
            safe.contracts.push(contract);
        };
        for (var i = 0; i < items.length; i++) {
            _loop_1(i);
        }
        this.recountNewContracts();
    };
    Wallet.prototype.recountNewContracts = function () {
        this.new_contracts = (this.contracts.filter(function (item) { return item.is_new === true; })).length;
    };
    Wallet.prototype.getContract = function (id) {
        for (var i = 0; i < this.contracts.length; i++) {
            if (this.contracts[i].contract_id === id) {
                return this.contracts[i];
            }
        }
        return null;
    };
    return Wallet;
}());



/***/ }),

/***/ "./src/app/_helpers/pipes/contract-status-messages.pipe.ts":
/*!*****************************************************************!*\
  !*** ./src/app/_helpers/pipes/contract-status-messages.pipe.ts ***!
  \*****************************************************************/
/*! exports provided: ContractStatusMessagesPipe */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ContractStatusMessagesPipe", function() { return ContractStatusMessagesPipe; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};

var ContractStatusMessagesPipe = /** @class */ (function () {
    function ContractStatusMessagesPipe() {
    }
    ContractStatusMessagesPipe.prototype.getStateSeller = function (stateNum) {
        var state = { part1: '', part2: '' };
        switch (stateNum) {
            case 1:
                state.part1 = 'New contract proposal';
                break;
            case 110:
                state.part1 = 'You ignored the contract proposal';
                break;
            case 201:
                state.part1 = 'You have accepted the contract proposal';
                state.part2 = 'Please wait for the pledges to be made';
                break;
            case 2:
                state.part1 = 'The buyer is waiting for the item to be delivered';
                state.part2 = 'Pledges made';
                break;
            case 3:
                state.part1 = 'Contract completed successfully';
                state.part2 = 'Item received, payment transferred, pledges returned';
                break;
            case 4:
                state.part1 = 'Item not received';
                state.part2 = 'All pledges nullified';
                break;
            case 5:
                state.part1 = 'New proposal to cancel contract and return pledges';
                break;
            case 601:
                state.part1 = 'The contract is being cancelled. Please wait for the pledge to be returned';
                break;
            case 6:
                state.part1 = 'Contract canceled';
                state.part2 = 'Pledges returned';
                break;
            case 130:
                state.part1 = 'You ignored the proposal to cancel the contract';
                break;
            case 140:
                state.part1 = 'The contract proposal has expired';
                break;
        }
        return state.part1 + ' ' + state.part2;
    };
    ContractStatusMessagesPipe.prototype.getStateCustomer = function (stateNum) {
        var state = { part1: '', part2: '' };
        switch (stateNum) {
            case 1:
                state.part1 = 'Waiting for seller respond to contract proposal';
                state.part2 = 'Pledge amount reserved';
                break;
            case 110:
                state.part1 = 'The seller ignored your contract proposal';
                state.part2 = 'Pledge amount unblocked';
                break;
            case 201:
                state.part1 = 'The seller accepted your contract proposal';
                state.part2 = 'Please wait for the pledges to be made';
                break;
            case 2:
                state.part1 = 'The seller accepted your contract proposal';
                state.part2 = 'Pledges made';
                break;
            case 120:
                state.part1 = 'Waiting for seller to ship item';
                state.part2 = 'Pledges made';
                break;
            case 3:
                state.part1 = 'Contract completed successfully';
                state.part2 = 'Item received, payment transferred, pledges returned';
                break;
            case 4:
                state.part1 = 'Item not received';
                state.part2 = 'All pledges nullified';
                break;
            case 5:
                state.part1 = 'Waiting for seller to respond to proposal to cancel contract and return pledges';
                break;
            case 601:
                state.part1 = 'The contract is being cancelled. Please wait for the pledge to be returned';
                break;
            case 6:
                state.part1 = 'Contract canceled';
                state.part2 = 'Pledges returned';
                break;
            case 130:
                state.part1 = 'The seller ignored your proposal to cancel the contract';
                break;
            case 140:
                state.part1 = 'The contract proposal has expired';
                break;
        }
        return state.part1 + ' ' + state.part2;
    };
    ContractStatusMessagesPipe.prototype.transform = function (item, args) {
        if (item.is_a) {
            return this.getStateCustomer(item.state);
        }
        else {
            return this.getStateSeller(item.state);
        }
    };
    ContractStatusMessagesPipe = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Pipe"])({
            name: 'contractStatusMessages'
        })
    ], ContractStatusMessagesPipe);
    return ContractStatusMessagesPipe;
}());



/***/ }),

/***/ "./src/app/_helpers/pipes/contract-time-left.pipe.ts":
/*!***********************************************************!*\
  !*** ./src/app/_helpers/pipes/contract-time-left.pipe.ts ***!
  \***********************************************************/
/*! exports provided: ContractTimeLeftPipe */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ContractTimeLeftPipe", function() { return ContractTimeLeftPipe; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _services_variables_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};



var ContractTimeLeftPipe = /** @class */ (function () {
    function ContractTimeLeftPipe(service, translate) {
        this.service = service;
        this.translate = translate;
    }
    ContractTimeLeftPipe.prototype.transform = function (value, arg) {
        var time = parseInt(((parseInt(value, 10) - this.service.exp_med_ts) / 3600).toFixed(0), 10);
        var type = arg || 0;
        if (time === 0) {
            return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_LESS_ONE');
        }
        if (this.service.settings.language === 'en') {
            if (type === 0) {
                if (time === 1) {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE', { time: time });
                }
                else {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY', { time: time });
                }
            }
            else if (type === 1) {
                if (time === 1) {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE_RESPONSE', { time: time });
                }
                else {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_RESPONSE', { time: time });
                }
            }
            else if (type === 2) {
                if (time === 1) {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE_WAITING', { time: time });
                }
                else {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_WAITING', { time: time });
                }
            }
        }
        else {
            var rest = time % 10;
            if (type === 0) {
                if (((time > 20) && (rest === 1)) || time === 1) {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE', { time: time });
                }
                else if ((time > 1) && (time < 5) || ((time > 20) && (rest === 2 || rest === 3 || rest === 4))) {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY', { time: time });
                }
                else {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_ALT', { time: time });
                }
            }
            else if (type === 1) {
                if (((time > 20) && (rest === 1)) || time === 1) {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE_RESPONSE', { time: time });
                }
                else if ((time > 1) && (time < 5) || ((time > 20) && (rest === 2 || rest === 3 || rest === 4))) {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_RESPONSE', { time: time });
                }
                else {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_ALT_RESPONSE', { time: time });
                }
            }
            else if (type === 2) {
                if (((time > 20) && (rest === 1)) || time === 1) {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE_WAITING', { time: time });
                }
                else if ((time > 1) && (time < 5) || ((time > 20) && (rest === 2 || rest === 3 || rest === 4))) {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_WAITING', { time: time });
                }
                else {
                    return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_ALT_WAITING', { time: time });
                }
            }
        }
        return null;
    };
    ContractTimeLeftPipe = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Pipe"])({
            name: 'contractTimeLeft'
        }),
        __metadata("design:paramtypes", [_services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"], _ngx_translate_core__WEBPACK_IMPORTED_MODULE_2__["TranslateService"]])
    ], ContractTimeLeftPipe);
    return ContractTimeLeftPipe;
}());



/***/ }),

/***/ "./src/app/_helpers/pipes/history-type-messages.pipe.ts":
/*!**************************************************************!*\
  !*** ./src/app/_helpers/pipes/history-type-messages.pipe.ts ***!
  \**************************************************************/
/*! exports provided: HistoryTypeMessagesPipe */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "HistoryTypeMessagesPipe", function() { return HistoryTypeMessagesPipe; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};

var HistoryTypeMessagesPipe = /** @class */ (function () {
    function HistoryTypeMessagesPipe() {
    }
    HistoryTypeMessagesPipe.prototype.transform = function (item, args) {
        if (item.tx_type === 0) {
            if (item.remote_addresses && item.remote_addresses[0]) {
                return item.remote_addresses[0];
            }
            else {
                if (item.is_income) {
                    return 'hidden';
                }
                else {
                    return 'Undefined';
                }
            }
        }
        else if (item.tx_type === 6 && item.height === 0) {
            return 'unknown';
        }
        else if (item.tx_type === 9) {
            if (item.hasOwnProperty('contract') && item.contract[0].is_a) {
                return 'Successfully complete contract, return remaining pledge';
            }
            else {
                return 'Successfully complete contract, receive payment on contract, and return pledge';
            }
        }
        else {
            switch (item.tx_type) {
                // case 0:
                //   return $filter('translate')('GUI_TX_TYPE_NORMAL');
                //   break;
                // case 1:
                //   return $filter('translate')('GUI_TX_TYPE_PUSH_OFFER');
                // case 2:
                //   return $filter('translate')('GUI_TX_TYPE_UPDATE_OFFER');
                // case 3:
                //   return $filter('translate')('GUI_TX_TYPE_CANCEL_OFFER');
                // case 4:
                //   return $filter('translate')('GUI_TX_TYPE_NEW_ALIAS');
                // case 5:
                //   return $filter('translate')('GUI_TX_TYPE_UPDATE_ALIAS');
                case 6:
                    return 'Mined funds';
                case 7:
                    return 'Send contract offer';
                case 8:
                    return 'Make pledge on offer';
                // case 9:
                //    return $filter('translate')('GUI_TX_TYPE_ESCROW_RELEASE_NORMAL');
                //    break;
                case 10:
                    return 'Nullify pledges for contract';
                case 11:
                    return 'Send proposal to cancel contract';
                case 12:
                    return 'Cancel contract, return pledges';
            }
        }
        return 'Undefined';
    };
    HistoryTypeMessagesPipe = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Pipe"])({
            name: 'historyTypeMessages'
        })
    ], HistoryTypeMessagesPipe);
    return HistoryTypeMessagesPipe;
}());



/***/ }),

/***/ "./src/app/_helpers/pipes/int-to-money.pipe.ts":
/*!*****************************************************!*\
  !*** ./src/app/_helpers/pipes/int-to-money.pipe.ts ***!
  \*****************************************************/
/*! exports provided: IntToMoneyPipe */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "IntToMoneyPipe", function() { return IntToMoneyPipe; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};

var IntToMoneyPipe = /** @class */ (function () {
    function IntToMoneyPipe() {
    }
    IntToMoneyPipe.prototype.transform = function (value, args) {
        if (value === 0 || value === undefined) {
            return '0';
        }
        var power = Math.pow(10, 8);
        var str = (value / power).toFixed(8);
        for (var i = str.length - 1; i >= 0; i--) {
            if (str[i] !== '0') {
                str = str.substr(0, i + 1);
                break;
            }
        }
        if (str[str.length - 1] === '.') {
            str = str.substr(0, str.length - 1);
        }
        return str;
    };
    IntToMoneyPipe = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Pipe"])({
            name: 'intToMoney'
        })
    ], IntToMoneyPipe);
    return IntToMoneyPipe;
}());



/***/ }),

/***/ "./src/app/_helpers/pipes/money-to-int.pipe.ts":
/*!*****************************************************!*\
  !*** ./src/app/_helpers/pipes/money-to-int.pipe.ts ***!
  \*****************************************************/
/*! exports provided: MoneyToIntPipe */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "MoneyToIntPipe", function() { return MoneyToIntPipe; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};

var MoneyToIntPipe = /** @class */ (function () {
    function MoneyToIntPipe() {
    }
    MoneyToIntPipe.prototype.transform = function (value, args) {
        var CURRENCY_DISPLAY_DECIMAL_POINT = 8;
        var result;
        if (value) {
            var am_str = value.toString().trim();
            var point_index = am_str.indexOf('.');
            var fraction_size = 0;
            if (-1 !== point_index) {
                fraction_size = am_str.length - point_index - 1;
                while (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size && '0' === am_str[am_str.length - 1]) {
                    am_str = am_str.slice(0, am_str.length - 1);
                    --fraction_size;
                }
                if (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size) {
                    return undefined;
                }
                am_str = am_str.slice(0, point_index) + am_str.slice(point_index + 1, am_str.length);
            }
            else {
                fraction_size = 0;
            }
            if (!am_str.length) {
                return undefined;
            }
            if (fraction_size < CURRENCY_DISPLAY_DECIMAL_POINT) {
                for (var i = 0; i !== CURRENCY_DISPLAY_DECIMAL_POINT - fraction_size; i++) {
                    am_str = am_str + '0';
                }
            }
            result = parseInt(am_str, 10);
        }
        return result;
    };
    MoneyToIntPipe = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Pipe"])({
            name: 'moneyToInt'
        })
    ], MoneyToIntPipe);
    return MoneyToIntPipe;
}());



/***/ }),

/***/ "./src/app/_helpers/services/backend.service.ts":
/*!******************************************************!*\
  !*** ./src/app/_helpers/services/backend.service.ts ***!
  \******************************************************/
/*! exports provided: BackendService */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "BackendService", function() { return BackendService; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var rxjs__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! rxjs */ "./node_modules/rxjs/_esm5/index.js");
/* harmony import */ var _variables_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ./variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../pipes/money-to-int.pipe */ "./src/app/_helpers/pipes/money-to-int.pipe.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};




var BackendService = /** @class */ (function () {
    function BackendService(variablesService, moneyToIntPipe) {
        this.variablesService = variablesService;
        this.moneyToIntPipe = moneyToIntPipe;
        this.backendLoaded = false;
    }
    BackendService.prototype.Debug = function (type, message) {
        switch (type) {
            case 0:
                console.error(message);
                break;
            case 1:
                console.warn(message);
                break;
            case 2:
                console.log(message);
                break;
            default:
                console.log(message);
                break;
        }
    };
    BackendService.prototype.informerRun = function (error, params, command) {
        var error_translate = '';
        switch (error) {
            case 'NOT_ENOUGH_MONEY':
                error_translate = 'ERROR.NOT_ENOUGH_MONEY';
                break;
            case 'CORE_BUSY':
                if (command !== 'get_all_aliases') {
                    error_translate = 'INFORMER.CORE_BUSY';
                }
                break;
            case 'OVERFLOW':
                if (command !== 'get_all_aliases') {
                    error_translate = '';
                }
                break;
            case 'INTERNAL_ERROR:daemon is busy':
                error_translate = 'INFORMER.DAEMON_BUSY';
                break;
            case 'INTERNAL_ERROR:not enough money':
            case 'INTERNAL_ERROR:NOT_ENOUGH_MONEY':
                if (command === 'cancel_offer') {
                    // error_translate = $filter('translate')('INFORMER.NO_MONEY_REMOVE_OFFER', {
                    //   'fee': CONFIG.standart_fee,
                    //   'currency': CONFIG.currency_symbol
                    // });
                }
                else {
                    error_translate = 'INFORMER.NO_MONEY';
                }
                break;
            case 'INTERNAL_ERROR:not enough outputs to mix':
                error_translate = 'MESSAGE.NOT_ENOUGH_OUTPUTS_TO_MIX';
                break;
            case 'INTERNAL_ERROR:transaction is too big':
                error_translate = 'MESSAGE.TRANSACTION_IS_TO_BIG';
                break;
            case 'INTERNAL_ERROR:Transfer attempt while daemon offline':
                error_translate = 'MESSAGE.TRANSFER_ATTEMPT';
                break;
            case 'ACCESS_DENIED':
                error_translate = 'INFORMER.ACCESS_DENIED';
                break;
            case 'INTERNAL_ERROR:transaction was rejected by daemon':
                if (command === 'request_alias_registration') {
                    error_translate = 'INFORMER.ALIAS_IN_REGISTER';
                }
                else {
                    error_translate = 'INFORMER.TRANSACTION_ERROR';
                }
                break;
            case 'INTERNAL_ERROR':
                error_translate = 'INFORMER.TRANSACTION_ERROR';
                break;
            case 'BAD_ARG':
                error_translate = 'INFORMER.BAD_ARG';
                break;
            case 'WALLET_WRONG_ID':
                error_translate = 'INFORMER.WALLET_WRONG_ID';
                break;
            case 'WRONG_PASSWORD':
            case 'WRONG_PASSWORD:invalid password':
                params = JSON.parse(params);
                if (!params.testEmpty) {
                    error_translate = 'INFORMER.WRONG_PASSWORD';
                }
                break;
            case 'FILE_RESTORED':
                if (command === 'open_wallet') {
                    // error_translate = $filter('translate')('INFORMER.FILE_RESTORED');
                }
                break;
            case 'FILE_NOT_FOUND':
                if (command !== 'open_wallet' && command !== 'get_alias_info_by_name' && command !== 'get_alias_info_by_address') {
                    // error_translate = $filter('translate')('INFORMER.FILE_NOT_FOUND');
                    params = JSON.parse(params);
                    if (params.path) {
                        error_translate += ': ' + params.path;
                    }
                }
                break;
            case 'CANCELED':
            case '':
                break;
            case 'FAIL':
                if (command === 'create_proposal' || command === 'accept_proposal' || command === 'release_contract' || command === 'request_cancel_contract' || command === 'accept_cancel_contract') {
                    error_translate = ' ';
                }
                break;
            case 'ALREADY_EXISTS':
                error_translate = 'INFORMER.FILE_EXIST';
                break;
            default:
                error_translate = error;
        }
        if (error.indexOf('FAIL:failed to save file') > -1) {
            error_translate = 'INFORMER.FILE_NOT_SAVED';
        }
        if (error_translate !== '') {
            alert(error_translate);
        }
    };
    BackendService.prototype.commandDebug = function (command, params, result) {
        this.Debug(2, '----------------- ' + command + ' -----------------');
        var debug = {
            _send_params: params,
            _result: result
        };
        this.Debug(2, debug);
        try {
            this.Debug(2, JSON.parse(result));
        }
        catch (e) {
            this.Debug(2, { response_data: result, error_code: 'OK' });
        }
    };
    BackendService.prototype.asVal = function (data) {
        return { v: data };
    };
    BackendService.prototype.backendCallback = function (resultStr, params, callback, command) {
        var Result = resultStr;
        if (command !== 'get_clipboard') {
            if (!resultStr || resultStr === '') {
                Result = {};
            }
            else {
                try {
                    Result = JSON.parse(resultStr);
                }
                catch (e) {
                    Result = { response_data: resultStr, error_code: 'OK' };
                }
            }
        }
        else {
            Result = {
                error_code: 'OK',
                response_data: Result
            };
        }
        var Status = (Result.error_code === 'OK' || Result.error_code === 'TRUE');
        if (!Status && Status !== undefined && Result.error_code !== undefined) {
            this.Debug(1, 'API error for command: "' + command + '". Error code: ' + Result.error_code);
        }
        var data = ((typeof Result === 'object') && 'response_data' in Result) ? Result.response_data : Result;
        var res_error_code = false;
        if (typeof Result === 'object' && 'error_code' in Result && Result.error_code !== 'OK' && Result.error_code !== 'TRUE' && Result.error_code !== 'FALSE') {
            this.informerRun(Result.error_code, params, command);
            res_error_code = Result.error_code;
        }
        // if ( command === 'get_offers_ex' ){
        //   Service.printLog( "get_offers_ex offers count "+((data.offers)?data.offers.length:0) );
        // }
        if (typeof callback === 'function') {
            callback(Status, data, res_error_code);
        }
        else {
            return data;
        }
    };
    BackendService.prototype.runCommand = function (command, params, callback) {
        if (this.backendObject) {
            var Action = this.backendObject[command];
            if (!Action) {
                this.Debug(0, 'Run Command Error! Command "' + command + '" don\'t found in backendObject');
            }
            else {
                var that_1 = this;
                params = (typeof params === 'string') ? params : JSON.stringify(params);
                if (params === undefined || params === '{}') {
                    Action(function (resultStr) {
                        that_1.commandDebug(command, params, resultStr);
                        return that_1.backendCallback(resultStr, params, callback, command);
                    });
                }
                else {
                    Action(params, function (resultStr) {
                        that_1.commandDebug(command, params, resultStr);
                        return that_1.backendCallback(resultStr, params, callback, command);
                    });
                }
            }
        }
    };
    BackendService.prototype.eventSubscribe = function (command, callback) {
        if (command === 'on_core_event') {
            this.backendObject[command].connect(callback);
        }
        else {
            this.backendObject[command].connect(function (str) {
                callback(JSON.parse(str));
            });
        }
    };
    BackendService.prototype.initService = function () {
        var _this = this;
        return new rxjs__WEBPACK_IMPORTED_MODULE_1__["Observable"](function (observer) {
            if (!_this.backendLoaded) {
                _this.backendLoaded = true;
                var that_2 = _this;
                window.QWebChannel(window.qt.webChannelTransport, function (channel) {
                    that_2.backendObject = channel.objects.mediator_object;
                    observer.next('ok');
                });
            }
            else {
                if (!_this.backendObject) {
                    observer.error('error');
                    observer.error('error');
                }
            }
        });
    };
    BackendService.prototype.webkitLaunchedScript = function () {
        return this.runCommand('webkit_launched_script');
    };
    BackendService.prototype.quitRequest = function () {
        return this.runCommand('on_request_quit');
    };
    BackendService.prototype.getAppData = function (callback) {
        this.runCommand('get_app_data', {}, callback);
    };
    BackendService.prototype.storeAppData = function (callback) {
        this.runCommand('store_app_data', this.variablesService.settings, callback);
    };
    BackendService.prototype.getSecureAppData = function (pass, callback) {
        this.runCommand('get_secure_app_data', pass, callback);
    };
    BackendService.prototype.storeSecureAppData = function (callback) {
        var _this = this;
        if (this.variablesService.appPass === '') {
            return callback(false);
        }
        var wallets = [];
        this.variablesService.wallets.forEach(function (wallet) {
            wallets.push({ name: wallet.name, pass: wallet.pass, path: wallet.path });
        });
        this.backendObject['store_secure_app_data'](JSON.stringify(wallets), this.variablesService.appPass, function (dataStore) {
            _this.backendCallback(dataStore, {}, callback, 'store_secure_app_data');
        });
    };
    BackendService.prototype.haveSecureAppData = function (callback) {
        this.runCommand('have_secure_app_data', {}, callback);
    };
    BackendService.prototype.saveFileDialog = function (caption, fileMask, default_path, callback) {
        var dir = default_path ? default_path : '/';
        var params = {
            caption: caption,
            filemask: fileMask,
            default_dir: dir
        };
        this.runCommand('show_savefile_dialog', params, callback);
    };
    BackendService.prototype.openFileDialog = function (caption, fileMask, default_path, callback) {
        var dir = default_path ? default_path : '/';
        var params = {
            caption: caption,
            filemask: fileMask,
            default_dir: dir
        };
        this.runCommand('show_openfile_dialog', params, callback);
    };
    BackendService.prototype.generateWallet = function (path, pass, callback) {
        var params = {
            path: path,
            pass: pass
        };
        this.runCommand('generate_wallet', params, callback);
    };
    BackendService.prototype.openWallet = function (path, pass, testEmpty, callback) {
        var params = {
            path: path,
            pass: pass
        };
        params['testEmpty'] = !!(testEmpty);
        this.runCommand('open_wallet', params, callback);
    };
    BackendService.prototype.closeWallet = function (wallet_id, callback) {
        this.runCommand('close_wallet', { wallet_id: wallet_id }, callback);
    };
    BackendService.prototype.getSmartSafeInfo = function (wallet_id, callback) {
        this.runCommand('get_smart_safe_info', { wallet_id: +wallet_id }, callback);
    };
    BackendService.prototype.runWallet = function (wallet_id, callback) {
        this.runCommand('run_wallet', { wallet_id: +wallet_id }, callback);
    };
    BackendService.prototype.isValidRestoreWalletText = function (text, callback) {
        this.runCommand('is_valid_restore_wallet_text', text, callback);
    };
    BackendService.prototype.restoreWallet = function (path, pass, restore_key, callback) {
        var params = {
            restore_key: restore_key,
            path: path,
            pass: pass
        };
        this.runCommand('restore_wallet', params, callback);
    };
    BackendService.prototype.sendMoney = function (from_wallet_id, to_address, amount, fee, mixin, comment, callback) {
        var params = {
            wallet_id: parseInt(from_wallet_id, 10),
            destinations: [
                {
                    address: to_address,
                    amount: amount
                }
            ],
            mixin_count: (mixin) ? parseInt(mixin, 10) : 0,
            lock_time: 0,
            fee: this.moneyToIntPipe.transform(fee),
            comment: comment,
            push_payer: true
        };
        this.runCommand('transfer', params, callback);
    };
    BackendService.prototype.validateAddress = function (address, callback) {
        this.runCommand('validate_address', address, callback);
    };
    BackendService.prototype.setClipboard = function (str, callback) {
        return this.runCommand('set_clipboard', str, callback);
    };
    BackendService.prototype.getClipboard = function (callback) {
        return this.runCommand('get_clipboard', {}, callback);
    };
    BackendService.prototype.createProposal = function (wallet_id, title, comment, a_addr, b_addr, to_pay, a_pledge, b_pledge, time, payment_id, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10),
            details: {
                t: title,
                c: comment,
                a_addr: a_addr,
                b_addr: b_addr,
                to_pay: this.moneyToIntPipe.transform(to_pay),
                a_pledge: this.moneyToIntPipe.transform(a_pledge) - this.moneyToIntPipe.transform(to_pay),
                b_pledge: this.moneyToIntPipe.transform(b_pledge)
            },
            payment_id: payment_id,
            expiration_period: parseInt(time, 10) * 60 * 60,
            fee: this.moneyToIntPipe.transform('0.01'),
            b_fee: this.moneyToIntPipe.transform('0.01')
        };
        this.Debug(1, params);
        this.runCommand('create_proposal', params, callback);
    };
    BackendService.prototype.getContracts = function (wallet_id, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10)
        };
        this.Debug(1, params);
        this.runCommand('get_contracts', params, callback);
    };
    BackendService.prototype.acceptProposal = function (wallet_id, contract_id, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10),
            contract_id: contract_id
        };
        this.Debug(1, params);
        this.runCommand('accept_proposal', params, callback);
    };
    BackendService.prototype.releaseProposal = function (wallet_id, contract_id, release_type, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10),
            contract_id: contract_id,
            release_type: release_type // "normal" or "burn"
        };
        this.Debug(1, params);
        this.runCommand('release_contract', params, callback);
    };
    BackendService.prototype.requestCancelContract = function (wallet_id, contract_id, time, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10),
            contract_id: contract_id,
            fee: this.moneyToIntPipe.transform('0.01'),
            expiration_period: parseInt(time, 10) * 60 * 60
        };
        this.Debug(1, params);
        this.runCommand('request_cancel_contract', params, callback);
    };
    BackendService.prototype.acceptCancelContract = function (wallet_id, contract_id, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10),
            contract_id: contract_id
        };
        this.Debug(1, params);
        this.runCommand('accept_cancel_contract', params, callback);
    };
    BackendService.prototype.getMiningHistory = function (wallet_id, callback) {
        this.runCommand('get_mining_history', { wallet_id: parseInt(wallet_id, 10) }, callback);
    };
    BackendService.prototype.startPosMining = function (wallet_id, callback) {
        this.runCommand('start_pos_mining', { wallet_id: parseInt(wallet_id, 10) }, callback);
    };
    BackendService.prototype.stopPosMining = function (wallet_id, callback) {
        this.runCommand('stop_pos_mining', { wallet_id: parseInt(wallet_id, 10) }, callback);
    };
    BackendService = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Injectable"])(),
        __metadata("design:paramtypes", [_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"], _pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_3__["MoneyToIntPipe"]])
    ], BackendService);
    return BackendService;
}());

/*

    var deferred = null;

    var Service = {







      /!*  API  *!/

      toggleAutoStart: function (value) {
        return this.runCommand('toggle_autostart', asVal(value));
      },



      getDefaultFee: function (callback) {
        return this.runCommand('get_default_fee', {}, callback);
      },

      getOptions: function (callback) {
        return this.runCommand('get_options', {}, callback);
      },

      isFileExist: function (path, callback) {
        return this.runCommand('is_file_exist', path, callback);
      },



      isAutoStartEnabled: function (callback) {
        this.runCommand('is_autostart_enabled', {}, function (status, data) {
          if (angular.isFunction(callback)) {
            callback('error_code' in data && data.error_code !== 'FALSE')
          }
        });
      },



      setLogLevel: function (level) {
        return this.runCommand('set_log_level', asVal(level))
      },

      resetWalletPass: function (wallet_id, pass, callback) {
        this.runCommand('reset_wallet_password', {wallet_id: wallet_id, pass: pass}, callback);
      },

      getVersion: function (callback) {
        this.runCommand('get_version', {}, function (status, version) {
          callback(version)
        })
      },

      getOsVersion: function (callback) {
        this.runCommand('get_os_version', {}, function (status, version) {
          callback(version)
        })
      },

      getLogFile: function (callback) {
        this.runCommand('get_log_file', {}, function (status, version) {
          callback(version)
        })
      },


      resync_wallet: function (wallet_id, callback) {
        this.runCommand('resync_wallet', {wallet_id: wallet_id}, callback);
      },





      registerAlias: function (wallet_id, alias, address, fee, comment, reward, callback) {
        var params = {
          "wallet_id": wallet_id,
          "alias": {
            "alias": alias,
            "address": address,
            "tracking_key": "",
            "comment": comment
          },
          "fee": $filter('money_to_int')(fee),
          "reward": $filter('money_to_int')(reward)
        };
        this.runCommand('request_alias_registration', params, callback);
      },

      updateAlias: function (wallet_id, alias, fee, callback) {
        var params = {
          wallet_id: wallet_id,
          alias: {
            "alias": alias.name.replace("@", ""),
            "address": alias.address,
            "tracking_key": "",
            "comment": alias.comment
          },
          fee: $filter('money_to_int')(fee)
        };
        this.runCommand('request_alias_update', params, callback);
      },

      getAllAliases: function (callback) {
        this.runCommand('get_all_aliases', {}, callback);
      },

      getAliasByName: function (value, callback) {
        return this.runCommand('get_alias_info_by_name', value, callback);
      },

      getAliasByAddress: function (value, callback) {
        return this.runCommand('get_alias_info_by_address', value, callback);
      },

      getPoolInfo: function (callback) {
        this.runCommand('get_tx_pool_info', {}, callback);
      },

      localization: function (stringsArray, title, callback) {
        var data = {
          strings: stringsArray,
          language_title: title
        };
        this.runCommand('set_localization_strings', data, callback);
      },

      storeFile: function (path, buff, callback) {
        this.backendObject['store_to_file'](path, (typeof buff === 'string' ? buff : JSON.stringify(buff)), function (data) {
          backendCallback(data, {}, callback, 'store_to_file');
        });
      },





      getMiningEstimate: function (amount_coins, time, callback) {
        var params = {
          "amount_coins": $filter('money_to_int')(amount_coins),
          "time": parseInt(time)
        };
        this.runCommand('get_mining_estimate', params, callback);
      },

      backupWalletKeys: function (wallet_id, path, callback) {
        var params = {
          "wallet_id": wallet_id,
          "path": path
        };
        this.runCommand('backup_wallet_keys', params, callback);
      },


      getAliasCoast: function (alias, callback) {
        this.runCommand('get_alias_coast', asVal(alias), callback);
      },




      setBlockedIcon: function (enabled, callback) {
        var mode = (enabled) ? "blocked" : "normal";
        Service.runCommand('bool_toggle_icon', mode, callback);
      },





      getWalletInfo: function (wallet_id, callback) {
        this.runCommand('get_wallet_info', {wallet_id: wallet_id}, callback);
      },



      printText: function (content) {
        return this.runCommand('print_text', {html_text: content});
      },

      printLog: function (msg, log_level) {
        return this.runCommand('print_log', {msg: msg, log_level: log_level});
      },

      openUrlInBrowser: function (url, callback) {
        return this.runCommand('open_url_in_browser', url, callback);
      },



      /!*  API END  *!/

    };
    return Service;
  }]);

})();

*/


/***/ }),

/***/ "./src/app/_helpers/services/variables.service.ts":
/*!********************************************************!*\
  !*** ./src/app/_helpers/services/variables.service.ts ***!
  \********************************************************/
/*! exports provided: VariablesService */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "VariablesService", function() { return VariablesService; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var rxjs__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! rxjs */ "./node_modules/rxjs/_esm5/index.js");
/* harmony import */ var idlejs_dist__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! idlejs/dist */ "./node_modules/idlejs/dist/index.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var ngx_contextmenu__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ngx-contextmenu */ "./node_modules/ngx-contextmenu/fesm5/ngx-contextmenu.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};





var VariablesService = /** @class */ (function () {
    function VariablesService(router, ngZone, contextMenuService) {
        var _this = this;
        this.router = router;
        this.ngZone = ngZone;
        this.contextMenuService = contextMenuService;
        this.appPass = '';
        this.moneyEquivalent = 0;
        this.defaultTheme = 'dark';
        this.defaultCurrency = 'ZAN';
        this.exp_med_ts = 0;
        this.height_app = 0;
        this.daemon_state = 0;
        this.sync = {
            progress_value: 0,
            progress_value_text: '0'
        };
        this.settings = {
            theme: '',
            language: 'en',
            default_path: '/',
            viewedContracts: [],
            notViewedContracts: []
        };
        this.wallets = [];
        this.getHeightAppEvent = new rxjs__WEBPACK_IMPORTED_MODULE_1__["BehaviorSubject"](null);
        this.getRefreshStackingEvent = new rxjs__WEBPACK_IMPORTED_MODULE_1__["BehaviorSubject"](null);
        this.idle = new idlejs_dist__WEBPACK_IMPORTED_MODULE_2__["Idle"]()
            .whenNotInteractive()
            .within(15)
            .do(function () {
            _this.ngZone.run(function () {
                _this.idle.stop();
                _this.appPass = '';
                _this.router.navigate(['/login'], { queryParams: { type: 'auth' } });
            });
        });
    }
    VariablesService.prototype.setHeightApp = function (height) {
        if (height !== this.height_app) {
            this.height_app = height;
            this.getHeightAppEvent.next(height);
        }
    };
    VariablesService.prototype.setRefreshStacking = function (wallet_id) {
        this.getHeightAppEvent.next(wallet_id);
    };
    VariablesService.prototype.setCurrentWallet = function (id) {
        var _this = this;
        this.wallets.forEach(function (wallet) {
            if (wallet.wallet_id === id) {
                _this.currentWallet = wallet;
            }
        });
    };
    VariablesService.prototype.getWallet = function (id) {
        for (var i = 0; i < this.wallets.length; i++) {
            if (this.wallets[i].wallet_id === id) {
                return this.wallets[i];
            }
        }
        return null;
    };
    VariablesService.prototype.startCountdown = function () {
        this.idle.restart();
    };
    VariablesService.prototype.stopCountdown = function () {
        this.idle.stop();
    };
    VariablesService.prototype.onContextMenu = function ($event) {
        $event.target['contextSelectionStart'] = $event.target['selectionStart'];
        $event.target['contextSelectionEnd'] = $event.target['selectionEnd'];
        if ($event.target && ($event.target['nodeName'].toUpperCase() === 'TEXTAREA' || $event.target['nodeName'].toUpperCase() === 'INPUT') && !$event.target['readOnly']) {
            this.contextMenuService.show.next({
                contextMenu: this.contextMenu,
                event: $event,
                item: $event.target,
            });
            $event.preventDefault();
            $event.stopPropagation();
        }
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", ngx_contextmenu__WEBPACK_IMPORTED_MODULE_4__["ContextMenuComponent"])
    ], VariablesService.prototype, "contextMenu", void 0);
    VariablesService = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Injectable"])({
            providedIn: 'root'
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_3__["Router"], _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"], ngx_contextmenu__WEBPACK_IMPORTED_MODULE_4__["ContextMenuService"]])
    ], VariablesService);
    return VariablesService;
}());



/***/ }),

/***/ "./src/app/app-routing.module.ts":
/*!***************************************!*\
  !*** ./src/app/app-routing.module.ts ***!
  \***************************************/
/*! exports provided: AppRoutingModule */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "AppRoutingModule", function() { return AppRoutingModule; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _main_main_component__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ./main/main.component */ "./src/app/main/main.component.ts");
/* harmony import */ var _login_login_component__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ./login/login.component */ "./src/app/login/login.component.ts");
/* harmony import */ var _wallet_wallet_component__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ./wallet/wallet.component */ "./src/app/wallet/wallet.component.ts");
/* harmony import */ var _send_send_component__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ./send/send.component */ "./src/app/send/send.component.ts");
/* harmony import */ var _receive_receive_component__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! ./receive/receive.component */ "./src/app/receive/receive.component.ts");
/* harmony import */ var _history_history_component__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! ./history/history.component */ "./src/app/history/history.component.ts");
/* harmony import */ var _contracts_contracts_component__WEBPACK_IMPORTED_MODULE_8__ = __webpack_require__(/*! ./contracts/contracts.component */ "./src/app/contracts/contracts.component.ts");
/* harmony import */ var _purchase_purchase_component__WEBPACK_IMPORTED_MODULE_9__ = __webpack_require__(/*! ./purchase/purchase.component */ "./src/app/purchase/purchase.component.ts");
/* harmony import */ var _messages_messages_component__WEBPACK_IMPORTED_MODULE_10__ = __webpack_require__(/*! ./messages/messages.component */ "./src/app/messages/messages.component.ts");
/* harmony import */ var _typing_message_typing_message_component__WEBPACK_IMPORTED_MODULE_11__ = __webpack_require__(/*! ./typing-message/typing-message.component */ "./src/app/typing-message/typing-message.component.ts");
/* harmony import */ var _staking_staking_component__WEBPACK_IMPORTED_MODULE_12__ = __webpack_require__(/*! ./staking/staking.component */ "./src/app/staking/staking.component.ts");
/* harmony import */ var _settings_settings_component__WEBPACK_IMPORTED_MODULE_13__ = __webpack_require__(/*! ./settings/settings.component */ "./src/app/settings/settings.component.ts");
/* harmony import */ var _create_wallet_create_wallet_component__WEBPACK_IMPORTED_MODULE_14__ = __webpack_require__(/*! ./create-wallet/create-wallet.component */ "./src/app/create-wallet/create-wallet.component.ts");
/* harmony import */ var _open_wallet_open_wallet_component__WEBPACK_IMPORTED_MODULE_15__ = __webpack_require__(/*! ./open-wallet/open-wallet.component */ "./src/app/open-wallet/open-wallet.component.ts");
/* harmony import */ var _restore_wallet_restore_wallet_component__WEBPACK_IMPORTED_MODULE_16__ = __webpack_require__(/*! ./restore-wallet/restore-wallet.component */ "./src/app/restore-wallet/restore-wallet.component.ts");
/* harmony import */ var _seed_phrase_seed_phrase_component__WEBPACK_IMPORTED_MODULE_17__ = __webpack_require__(/*! ./seed-phrase/seed-phrase.component */ "./src/app/seed-phrase/seed-phrase.component.ts");
/* harmony import */ var _wallet_details_wallet_details_component__WEBPACK_IMPORTED_MODULE_18__ = __webpack_require__(/*! ./wallet-details/wallet-details.component */ "./src/app/wallet-details/wallet-details.component.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};


// Components

















var routes = [
    {
        path: '',
        component: _main_main_component__WEBPACK_IMPORTED_MODULE_2__["MainComponent"]
    },
    {
        path: 'main',
        component: _main_main_component__WEBPACK_IMPORTED_MODULE_2__["MainComponent"]
    },
    {
        path: 'login',
        component: _login_login_component__WEBPACK_IMPORTED_MODULE_3__["LoginComponent"]
    },
    {
        path: 'wallet/:id',
        component: _wallet_wallet_component__WEBPACK_IMPORTED_MODULE_4__["WalletComponent"],
        children: [
            {
                path: 'send',
                component: _send_send_component__WEBPACK_IMPORTED_MODULE_5__["SendComponent"]
            },
            {
                path: 'receive',
                component: _receive_receive_component__WEBPACK_IMPORTED_MODULE_6__["ReceiveComponent"]
            },
            {
                path: 'history',
                component: _history_history_component__WEBPACK_IMPORTED_MODULE_7__["HistoryComponent"]
            },
            {
                path: 'contracts',
                component: _contracts_contracts_component__WEBPACK_IMPORTED_MODULE_8__["ContractsComponent"],
            },
            {
                path: 'purchase',
                component: _purchase_purchase_component__WEBPACK_IMPORTED_MODULE_9__["PurchaseComponent"]
            },
            {
                path: 'purchase/:id',
                component: _purchase_purchase_component__WEBPACK_IMPORTED_MODULE_9__["PurchaseComponent"]
            },
            {
                path: 'messages',
                component: _messages_messages_component__WEBPACK_IMPORTED_MODULE_10__["MessagesComponent"],
            },
            {
                path: 'messages/:id',
                component: _typing_message_typing_message_component__WEBPACK_IMPORTED_MODULE_11__["TypingMessageComponent"],
            },
            {
                path: 'staking',
                component: _staking_staking_component__WEBPACK_IMPORTED_MODULE_12__["StakingComponent"]
            },
            {
                path: '',
                redirectTo: 'send',
                pathMatch: 'full'
            }
        ]
    },
    {
        path: 'create',
        component: _create_wallet_create_wallet_component__WEBPACK_IMPORTED_MODULE_14__["CreateWalletComponent"]
    },
    {
        path: 'open',
        component: _open_wallet_open_wallet_component__WEBPACK_IMPORTED_MODULE_15__["OpenWalletComponent"]
    },
    {
        path: 'restore',
        component: _restore_wallet_restore_wallet_component__WEBPACK_IMPORTED_MODULE_16__["RestoreWalletComponent"]
    },
    {
        path: 'seed-phrase',
        component: _seed_phrase_seed_phrase_component__WEBPACK_IMPORTED_MODULE_17__["SeedPhraseComponent"]
    },
    {
        path: 'details',
        component: _wallet_details_wallet_details_component__WEBPACK_IMPORTED_MODULE_18__["WalletDetailsComponent"]
    },
    {
        path: 'settings',
        component: _settings_settings_component__WEBPACK_IMPORTED_MODULE_13__["SettingsComponent"]
    },
    {
        path: '',
        redirectTo: '/',
        pathMatch: 'full'
    }
];
var AppRoutingModule = /** @class */ (function () {
    function AppRoutingModule() {
    }
    AppRoutingModule = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["NgModule"])({
            imports: [_angular_router__WEBPACK_IMPORTED_MODULE_1__["RouterModule"].forRoot(routes)],
            exports: [_angular_router__WEBPACK_IMPORTED_MODULE_1__["RouterModule"]]
        })
    ], AppRoutingModule);
    return AppRoutingModule;
}());



/***/ }),

/***/ "./src/app/app.component.html":
/*!************************************!*\
  !*** ./src/app/app.component.html ***!
  \************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<app-sidebar *ngIf=\"variablesService.appPass\"></app-sidebar>\r\n\r\n<div class=\"app-content scrolled-content\">\r\n  <router-outlet></router-outlet>\r\n</div>\r\n\r\n<context-menu>\r\n  <ng-template contextMenuItem (execute)=\"contextMenuCopy($event.item)\">copy</ng-template>\r\n  <ng-template contextMenuItem (execute)=\"contextMenuPaste($event.item)\">paste</ng-template>\r\n  <ng-template contextMenuItem (execute)=\"contextMenuSelect($event.item)\">select all</ng-template>\r\n</context-menu>\r\n"

/***/ }),

/***/ "./src/app/app.component.scss":
/*!************************************!*\
  !*** ./src/app/app.component.scss ***!
  \************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "/*\r\n* Implementation of themes\r\n*/\n.app-content {\n  display: flex;\n  overflow-x: overlay;\n  overflow-y: hidden;\n  width: 100%; }\n\r\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvRDpcXHphbm8vc3JjXFxhc3NldHNcXHNjc3NcXGJhc2VcXF9taXhpbnMuc2NzcyIsInNyYy9hcHAvRDpcXHphbm8vc3JjXFxhcHBcXGFwcC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUE4RUE7O0VBRUU7QUM5RUY7RUFDRSxjQUFhO0VBQ2Isb0JBQW1CO0VBQ25CLG1CQUFrQjtFQUNsQixZQUFXLEVBQ1oiLCJmaWxlIjoic3JjL2FwcC9hcHAuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyJAbWl4aW4gdGV4dC10cnVuY2F0ZSB7XHJcbiAgb3ZlcmZsb3c6IGhpZGRlbjtcclxuICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcclxuICB3aGl0ZS1zcGFjZTogbm93cmFwO1xyXG59XHJcbkBtaXhpbiB0ZXh0V3JhcCB7XHJcbiAgd2hpdGUtc3BhY2U6IG5vcm1hbDtcclxuICBvdmVyZmxvdy13cmFwOiBicmVhay13b3JkO1xyXG4gIHdvcmQtd3JhcDogYnJlYWstd29yZDtcclxuICB3b3JkLWJyZWFrOiBicmVhay1hbGw7XHJcbiAgbGluZS1icmVhazogc3RyaWN0O1xyXG4gIC13ZWJraXQtaHlwaGVuczogYXV0bztcclxuICAtbXMtaHlwaGVuczogYXV0bztcclxuICBoeXBoZW5zOiBhdXRvO1xyXG59XHJcbkBtaXhpbiBjb3ZlckJveCB7XHJcblx0cG9zaXRpb246IGFic29sdXRlO1xyXG5cdHRvcDogMDtcclxuXHRsZWZ0OiAwO1xyXG5cdHdpZHRoOiAxMDAlO1xyXG5cdGhlaWdodDogMTAwJTtcclxufVxyXG5AbWl4aW4gYWJzICgkdG9wOiBhdXRvLCAkcmlnaHQ6IGF1dG8sICRib3R0b206IGF1dG8sICRsZWZ0OiBhdXRvKSB7XHJcbiAgdG9wOiAkdG9wO1xyXG4gIHJpZ2h0OiAkcmlnaHQ7XHJcbiAgYm90dG9tOiAkYm90dG9tO1xyXG4gIGxlZnQ6ICRsZWZ0O1xyXG4gIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxufVxyXG5AbWl4aW4gY292ZXJJbWcge1xyXG5cdGJhY2tncm91bmQtcmVwZWF0OiBuby1yZXBlYXQ7XHJcblx0LXdlYmtpdC1iYWNrZ3JvdW5kLXNpemU6IGNvdmVyO1xyXG5cdC1vLWJhY2tncm91bmQtc2l6ZTogY292ZXI7XHJcblx0YmFja2dyb3VuZC1zaXplOiBjb3ZlcjtcclxuXHRiYWNrZ3JvdW5kLXBvc2l0aW9uOiA1MCUgNTAlO1xyXG59XHJcbkBtaXhpbiB2YWxpbmdCb3gge1xyXG5cdHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuXHR0b3A6ICA1MCU7XHJcblx0bGVmdDogNTAlO1xyXG5cdHRyYW5zZm9ybTogdHJhbnNsYXRlKC01MCUsIC01MCUpO1xyXG59XHJcbkBtaXhpbiB1blNlbGVjdCB7XHJcblx0LXdlYmtpdC10b3VjaC1jb2xsb3V0OiBub25lO1xyXG4gIC13ZWJraXQtdXNlci1zZWxlY3Q6IG5vbmU7XHJcbiAgLWtodG1sLXVzZXItc2VsZWN0OiBub25lO1xyXG4gIC1tb3otdXNlci1zZWxlY3Q6IG5vbmU7XHJcbiAgLW1zLXVzZXItc2VsZWN0OiBub25lO1xyXG4gIHVzZXItc2VsZWN0OiBub25lO1xyXG59XHJcbkBtaXhpbiBtYXgxMTk5IHsgLy8gbWFrZXQgMTE3MVxyXG4gIEBtZWRpYSAobWF4LXdpZHRoOiAxMTk5cHgpIHsgQGNvbnRlbnQ7IH1cclxufVxyXG5AbWl4aW4gbWF4MTE3MCB7IC8vIG1ha2V0cyA5OTJcclxuICBAbWVkaWEgKG1heC13aWR0aDogMTE3MHB4KSB7IEBjb250ZW50OyB9XHJcbn1cclxuQG1peGluIG1heDk5MSB7IC8vIG1ha2V0cyA3NjJcclxuICBAbWVkaWEgKG1heC13aWR0aDogOTkxcHgpIHsgQGNvbnRlbnQ7IH1cclxufVxyXG5AbWl4aW4gbWF4NzYxIHsgLy8gbWFrZXRzIDU3NlxyXG4gIEBtZWRpYSAobWF4LXdpZHRoOiA3NjFweCkgeyBAY29udGVudDsgfVxyXG59XHJcbkBtaXhpbiBtYXg1NzUgeyAvLyBtYWtldHMgNDAwXHJcbiAgQG1lZGlhIChtYXgtd2lkdGg6IDU3NXB4KSB7IEBjb250ZW50OyB9XHJcbn1cclxuQG1peGluIG1vYmlsZSB7XHJcbiAgQG1lZGlhIChtYXgtd2lkdGg6IDM5OXB4KSB7IEBjb250ZW50OyB9XHJcbn1cclxuQG1peGluIGljb0NlbnRlciB7XHJcbiAgICBiYWNrZ3JvdW5kLXJlcGVhdDogbm8tcmVwZWF0O1xyXG4gICAgYmFja2dyb3VuZC1wb3NpdGlvbjogY2VudGVyIGNlbnRlcjtcclxufVxyXG5AbWl4aW4gcHNldWRvICgkZGlzcGxheTogYmxvY2ssICRwb3M6IGFic29sdXRlLCAkY29udGVudDogJycpe1xyXG4gIGNvbnRlbnQ6ICRjb250ZW50O1xyXG4gIGRpc3BsYXk6ICRkaXNwbGF5O1xyXG4gIHBvc2l0aW9uOiAkcG9zO1xyXG59XHJcblxyXG4vKlxyXG4qIEltcGxlbWVudGF0aW9uIG9mIHRoZW1lc1xyXG4qL1xyXG5AbWl4aW4gdGhlbWlmeSgkdGhlbWVzOiAkdGhlbWVzKSB7XHJcbiAgQGVhY2ggJHRoZW1lLCAkbWFwIGluICR0aGVtZXMge1xyXG4gICAgLnRoZW1lLSN7JHRoZW1lfSAmIHtcclxuICAgICAgJHRoZW1lLW1hcDogKCkgIWdsb2JhbDtcclxuICAgICAgQGVhY2ggJGtleSwgJHN1Ym1hcCBpbiAkbWFwIHtcclxuICAgICAgICAkdmFsdWU6IG1hcC1nZXQobWFwLWdldCgkdGhlbWVzLCAkdGhlbWUpLCAnI3ska2V5fScpO1xyXG4gICAgICAgICR0aGVtZS1tYXA6IG1hcC1tZXJnZSgkdGhlbWUtbWFwLCAoJGtleTogJHZhbHVlKSkgIWdsb2JhbDtcclxuICAgICAgfVxyXG4gICAgICBAY29udGVudDtcclxuICAgICAgJHRoZW1lLW1hcDogbnVsbCAhZ2xvYmFsO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG5cclxuQGZ1bmN0aW9uIHRoZW1lZCgka2V5KSB7XHJcbiAgQHJldHVybiBtYXAtZ2V0KCR0aGVtZS1tYXAsICRrZXkpO1xyXG59XHJcbiIsIkBpbXBvcnQgJ35zcmMvYXNzZXRzL3Njc3MvYmFzZS9taXhpbnMnO1xyXG5cclxuLmFwcC1jb250ZW50IHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIG92ZXJmbG93LXg6IG92ZXJsYXk7XHJcbiAgb3ZlcmZsb3cteTogaGlkZGVuO1xyXG4gIHdpZHRoOiAxMDAlO1xyXG59XHJcbiJdfQ== */"

/***/ }),

/***/ "./src/app/app.component.ts":
/*!**********************************!*\
  !*** ./src/app/app.component.ts ***!
  \**********************************/
/*! exports provided: AppComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "AppComponent", function() { return AppComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_common_http__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/common/http */ "./node_modules/@angular/common/fesm5/http.js");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ./_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ./_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var AppComponent = /** @class */ (function () {
    function AppComponent(http, renderer, translate, backend, router, variablesService, ngZone) {
        this.http = http;
        this.renderer = renderer;
        this.translate = translate;
        this.backend = backend;
        this.router = router;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.onQuitRequest = false;
        translate.addLangs(['en', 'fr']);
        translate.setDefaultLang('en');
        // const browserLang = translate.getBrowserLang();
        // translate.use(browserLang.match(/en|fr/) ? browserLang : 'en');
    }
    AppComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.backend.initService().subscribe(function (initMessage) {
            console.log('Init message: ', initMessage);
            _this.backend.webkitLaunchedScript();
            _this.backend.eventSubscribe('quit_requested', function () {
                if (!_this.onQuitRequest) {
                    _this.backend.storeSecureAppData(function () {
                        _this.backend.storeAppData(function () {
                            var recursionCloseWallets = function () {
                                if (_this.variablesService.wallets.length) {
                                    var lastIndex_1 = _this.variablesService.wallets.length - 1;
                                    _this.backend.closeWallet(_this.variablesService.wallets[lastIndex_1].wallet_id, function () {
                                        _this.variablesService.wallets.splice(lastIndex_1, 1);
                                        recursionCloseWallets();
                                    });
                                }
                                else {
                                    _this.backend.quitRequest();
                                }
                            };
                            recursionCloseWallets();
                        });
                    });
                }
                _this.onQuitRequest = true;
            });
            _this.backend.eventSubscribe('update_wallet_status', function (data) {
                console.log('----------------- update_wallet_status -----------------');
                console.log(data);
                var wallet_state = data.wallet_state;
                var is_mining = data.is_mining;
                var wallet = _this.variablesService.getWallet(data.wallet_id);
                // 1-synch, 2-ready, 3 - error
                if (wallet) {
                    _this.ngZone.run(function () {
                        wallet.loaded = false;
                        wallet.staking = is_mining;
                        if (wallet_state === 2) { // ready
                            wallet.loaded = true;
                        }
                        if (wallet_state === 3) { // error
                            // safe.error = true;
                        }
                        wallet.balance = data.balance;
                        wallet.unlocked_balance = data.unlocked_balance;
                        wallet.mined_total = data.minied_total;
                    });
                }
            });
            _this.backend.eventSubscribe('wallet_sync_progress', function (data) {
                console.log('----------------- wallet_sync_progress -----------------');
                console.log(data);
                var wallet = _this.variablesService.getWallet(data.wallet_id);
                if (wallet) {
                    _this.ngZone.run(function () {
                        wallet.progress = data.progress;
                        if (wallet.progress === 0) {
                            wallet.loaded = false;
                        }
                        else if (wallet.progress === 100) {
                            wallet.loaded = true;
                        }
                    });
                }
            });
            _this.backend.eventSubscribe('update_daemon_state', function (data) {
                console.log('----------------- update_daemon_state -----------------');
                console.log('DAEMON:' + data.daemon_network_state);
                console.log(data);
                _this.variablesService.exp_med_ts = data['expiration_median_timestamp'] + 600 + 1;
                // this.variablesService.height_app = data.height;
                _this.variablesService.setHeightApp(data.height);
                _this.ngZone.run(function () {
                    _this.variablesService.daemon_state = data.daemon_network_state;
                    if (data.daemon_network_state === 1) {
                        var max = data.max_net_seen_height - data.synchronization_start_height;
                        var current = data.height - data.synchronization_start_height;
                        var return_val = Math.floor((current * 100 / max) * 100) / 100;
                        if (max === 0 || return_val < 0) {
                            _this.variablesService.sync.progress_value = 0;
                            _this.variablesService.sync.progress_value_text = '0.00';
                        }
                        else if (return_val >= 100) {
                            _this.variablesService.sync.progress_value = 100;
                            _this.variablesService.sync.progress_value_text = '99.99';
                        }
                        else {
                            _this.variablesService.sync.progress_value = return_val;
                            _this.variablesService.sync.progress_value_text = return_val.toFixed(2);
                        }
                    }
                });
            });
            _this.backend.eventSubscribe('money_transfer', function (data) {
                console.log('----------------- money_transfer -----------------');
                console.log(data);
                if (!data.ti) {
                    return;
                }
                var wallet_id = data.wallet_id;
                var tr_info = data.ti;
                var safe = _this.variablesService.getWallet(wallet_id);
                if (safe) {
                    _this.ngZone.run(function () {
                        if (!safe.loaded) {
                            safe.balance = data.balance;
                            safe.unlocked_balance = data.unlocked_balance;
                        }
                        else {
                            safe.balance = data.balance;
                            safe.unlocked_balance = data.unlocked_balance;
                        }
                        if (tr_info.tx_type === 6) {
                            _this.variablesService.setRefreshStacking(wallet_id);
                        }
                        var tr_exists = safe.excluded_history.some(function (elem) { return elem.tx_hash === tr_info.tx_hash; });
                        tr_exists = (!tr_exists) ? safe.history.some(function (elem) { return elem.tx_hash === tr_info.tx_hash; }) : tr_exists;
                        safe.prepareHistory([tr_info]);
                        if (tr_info.hasOwnProperty('contract')) {
                            var exp_med_ts = _this.variablesService.exp_med_ts;
                            var height_app = _this.variablesService.height_app;
                            var contract_1 = tr_info.contract[0];
                            if (tr_exists) {
                                for (var i = 0; i < safe.contracts.length; i++) {
                                    if (safe.contracts[i].contract_id === contract_1.contract_id && safe.contracts[i].is_a === contract_1.is_a) {
                                        safe.contracts[i].cancel_expiration_time = contract_1.cancel_expiration_time;
                                        safe.contracts[i].expiration_time = contract_1.expiration_time;
                                        safe.contracts[i].height = contract_1.height;
                                        safe.contracts[i].timestamp = contract_1.timestamp;
                                        break;
                                    }
                                }
                                // $rootScope.getContractsRecount();
                                return;
                            }
                            if (contract_1.state === 1 && contract_1.expiration_time < exp_med_ts) {
                                contract_1.state = 110;
                            }
                            else if (contract_1.state === 5 && contract_1.cancel_expiration_time < exp_med_ts) {
                                contract_1.state = 130;
                            }
                            else if (contract_1.state === 1) {
                                var searchResult2 = _this.variablesService.settings.notViewedContracts.find(function (elem) { return elem.state === 110 && elem.is_a === contract_1.is_a && elem.contract_id === contract_1.contract_id; });
                                if (searchResult2) {
                                    if (searchResult2.time === contract_1.expiration_time) {
                                        contract_1.state = 110;
                                    }
                                    else {
                                        for (var j = 0; j < _this.variablesService.settings.notViewedContracts.length; j++) {
                                            if (_this.variablesService.settings.notViewedContracts[j].contract_id === contract_1.contract_id && _this.variablesService.settings.notViewedContracts[j].is_a === contract_1.is_a) {
                                                _this.variablesService.settings.notViewedContracts.splice(j, 1);
                                                break;
                                            }
                                        }
                                        for (var j = 0; j < _this.variablesService.settings.viewedContracts.length; j++) {
                                            if (_this.variablesService.settings.viewedContracts[j].contract_id === contract_1.contract_id && _this.variablesService.settings.viewedContracts[j].is_a === contract_1.is_a) {
                                                _this.variablesService.settings.viewedContracts.splice(j, 1);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            else if (contract_1.state === 2 && (contract_1.height === 0 || (height_app - contract_1.height) < 10)) {
                                contract_1.state = 201;
                            }
                            else if (contract_1.state === 2) {
                                var searchResult3 = _this.variablesService.settings.viewedContracts.some(function (elem) { return elem.state === 120 && elem.is_a === contract_1.is_a && elem.contract_id === contract_1.contract_id; });
                                if (searchResult3) {
                                    contract_1.state = 120;
                                }
                            }
                            else if (contract_1.state === 5) {
                                var searchResult4 = _this.variablesService.settings.notViewedContracts.find(function (elem) { return elem.state === 130 && elem.is_a === contract_1.is_a && elem.contract_id === contract_1.contract_id; });
                                if (searchResult4) {
                                    if (searchResult4.time === contract_1.cancel_expiration_time) {
                                        contract_1.state = 130;
                                    }
                                    else {
                                        for (var j = 0; j < _this.variablesService.settings.notViewedContracts.length; j++) {
                                            if (_this.variablesService.settings.notViewedContracts[j].contract_id === contract_1.contract_id && _this.variablesService.settings.notViewedContracts[j].is_a === contract_1.is_a) {
                                                _this.variablesService.settings.notViewedContracts.splice(j, 1);
                                                break;
                                            }
                                        }
                                        for (var j = 0; j < _this.variablesService.settings.viewedContracts.length; j++) {
                                            if (_this.variablesService.settings.viewedContracts[j].contract_id === contract_1.contract_id && _this.variablesService.settings.viewedContracts[j].is_a === contract_1.is_a) {
                                                _this.variablesService.settings.viewedContracts.splice(j, 1);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            else if (contract_1.state === 6 && (contract_1.height === 0 || (height_app - contract_1.height) < 10)) {
                                contract_1.state = 601;
                            }
                            var searchResult = _this.variablesService.settings.viewedContracts.some(function (elem) { return elem.state === contract_1.state && elem.is_a === contract_1.is_a && elem.contract_id === contract_1.contract_id; });
                            contract_1.is_new = !searchResult;
                            contract_1['private_detailes'].a_pledge += contract_1['private_detailes'].to_pay;
                            var findContract = false;
                            for (var i = 0; i < safe.contracts.length; i++) {
                                if (safe.contracts[i].contract_id === contract_1.contract_id && safe.contracts[i].is_a === contract_1.is_a) {
                                    for (var prop in contract_1) {
                                        if (contract_1.hasOwnProperty(prop)) {
                                            safe.contracts[i][prop] = contract_1[prop];
                                        }
                                    }
                                    findContract = true;
                                    break;
                                }
                            }
                            if (findContract === false) {
                                safe.contracts.push(contract_1);
                            }
                            safe.recountNewContracts();
                        }
                    });
                }
            });
            _this.backend.eventSubscribe('money_transfer_cancel', function (data) {
                console.log('----------------- money_transfer_cancel -----------------');
                console.log(data);
                // if (!data.ti) {
                //   return;
                // }
                //
                // var wallet_id = data.wallet_id;
                // var tr_info = data.ti;
                // var safe = $rootScope.getSafeById(wallet_id);
                // if (safe) {
                //   if ( tr_info.hasOwnProperty("contract") ){
                //     for (var i = 0; i < $rootScope.contracts.length; i++) {
                //       if ($rootScope.contracts[i].contract_id === tr_info.contract[0].contract_id && $rootScope.contracts[i].is_a === tr_info.contract[0].is_a) {
                //         if ($rootScope.contracts[i].state === 1 || $rootScope.contracts[i].state === 110) {
                //           $rootScope.contracts[i].isNew = true;
                //           $rootScope.contracts[i].state = 140;
                //           $rootScope.getContractsRecount(); //escrow_code
                //         }
                //         break;
                //       }
                //     }
                //   }
                //   angular.forEach(safe.history, function (tr_item, key) {
                //     if (tr_item.tx_hash === tr_info.tx_hash) {
                //       safe.history.splice(key, 1);
                //     }
                //   });
                //
                //   var error_tr = '';
                //   switch (tr_info.tx_type) {
                //     case 0:
                //       error_tr = $filter('translate')('ERROR_GUI_TX_TYPE_NORMAL') + '<br>' +
                //         tr_info.tx_hash + '<br>' + safe.name + '<br>' + safe.address + '<br>' +
                //         $filter('translate')('ERROR_GUI_TX_TYPE_NORMAL_TO') + ' ' + $rootScope.moneyParse(tr_info.amount) + ' ' +
                //         $filter('translate')('ERROR_GUI_TX_TYPE_NORMAL_END');
                //       informer.error(error_tr);
                //       break;
                //     case 1:
                //       informer.error('ERROR_GUI_TX_TYPE_PUSH_OFFER');
                //       break;
                //     case 2:
                //       informer.error('ERROR_GUI_TX_TYPE_UPDATE_OFFER');
                //       break;
                //     case 3:
                //       informer.error('ERROR_GUI_TX_TYPE_CANCEL_OFFER');
                //       break;
                //     case 4:
                //       error_tr = $filter('translate')('ERROR_GUI_TX_TYPE_NEW_ALIAS') + '<br>' +
                //         tr_info.tx_hash + '<br>' + safe.name + '<br>' + safe.address + '<br>' +
                //         $filter('translate')('ERROR_GUI_TX_TYPE_NEW_ALIAS_END');
                //       informer.error(error_tr);
                //       break;
                //     case 5:
                //       error_tr = $filter('translate')('ERROR_GUI_TX_TYPE_UPDATE_ALIAS') + '<br>' +
                //         tr_info.tx_hash + '<br>' + safe.name + '<br>' + safe.address + '<br>' +
                //         $filter('translate')('ERROR_GUI_TX_TYPE_NEW_ALIAS_END');
                //       informer.error(error_tr);
                //       break;
                //     case 6:
                //       informer.error('ERROR_GUI_TX_TYPE_COIN_BASE');
                //       break;
                //   }
                // }
            });
            _this.intervalUpdateContractsState = setInterval(function () {
                _this.variablesService.wallets.forEach(function (wallet) {
                    wallet.contracts.forEach(function (contract) {
                        if (contract.state === 201 && contract.height !== 0 && (_this.variablesService.height_app - contract.height) >= 10) {
                            contract.state = 2;
                            contract.is_new = true;
                            console.warn('need check state in contracts');
                        }
                        else if (contract.state === 601 && contract.height !== 0 && (_this.variablesService.height_app - contract.height) >= 10) {
                            contract.state = 6;
                            contract.is_new = true;
                        }
                    });
                });
            }, 30000);
            _this.backend.getAppData(function (status, data) {
                if (data && Object.keys(data).length > 0) {
                    _this.variablesService.settings = data;
                    if (_this.variablesService.settings.hasOwnProperty('theme') && ['dark', 'white', 'gray'].indexOf(_this.variablesService.settings.theme) !== -1) {
                        _this.renderer.addClass(document.body, 'theme-' + _this.variablesService.settings.theme);
                    }
                    else {
                        _this.renderer.addClass(document.body, 'theme-' + _this.variablesService.defaultTheme);
                    }
                }
                else {
                    _this.variablesService.settings.theme = _this.variablesService.defaultTheme;
                    _this.renderer.addClass(document.body, 'theme-' + _this.variablesService.settings.theme);
                }
                if (_this.router.url !== '/login') {
                    _this.backend.haveSecureAppData(function (statusPass) {
                        if (statusPass) {
                            _this.ngZone.run(function () {
                                _this.router.navigate(['/login'], { queryParams: { type: 'auth' } });
                            });
                        }
                        else {
                            _this.ngZone.run(function () {
                                _this.router.navigate(['/login'], { queryParams: { type: 'reg' } });
                            });
                        }
                    });
                }
            });
        }, function (error) {
            console.log(error);
        });
        this.getMoneyEquivalent();
    };
    AppComponent.prototype.getMoneyEquivalent = function () {
        var _this = this;
        // todo now start only once, need check daemon state and re-init
        this.http.get('https://api.coinmarketcap.com/v2/ticker/2').subscribe(function (result) {
            if (result.hasOwnProperty('data')) {
                _this.variablesService.moneyEquivalent = result['data']['quotes']['USD']['price'];
            }
        }, function (error) {
            console.warn('Error coinmarketcap', error);
        });
    };
    AppComponent.prototype.contextMenuCopy = function (target) {
        if (target && (target['nodeName'].toUpperCase() === 'TEXTAREA' || target['nodeName'].toUpperCase() === 'INPUT')) {
            var start = (target['contextSelectionStart']) ? 'contextSelectionStart' : 'selectionStart';
            var end = (target['contextSelectionEnd']) ? 'contextSelectionEnd' : 'selectionEnd';
            var canUseSelection = ((target[start]) || (target[start] === '0'));
            var SelectedText = (canUseSelection) ? target['value'].substring(target[start], target[end]) : target['value'];
            this.backend.setClipboard(String(SelectedText));
        }
    };
    AppComponent.prototype.contextMenuPaste = function (target) {
        if (target && (target['nodeName'].toUpperCase() === 'TEXTAREA' || target['nodeName'].toUpperCase() === 'INPUT')) {
            this.backend.getClipboard(function (status, clipboard) {
                clipboard = String(clipboard);
                if (typeof clipboard !== 'string' || clipboard.length) {
                    var start = (target['contextSelectionStart']) ? 'contextSelectionStart' : 'selectionStart';
                    var end = (target['contextSelectionEnd']) ? 'contextSelectionEnd' : 'selectionEnd';
                    var canUseSelection = ((target[start]) || (target[start] === '0'));
                    var _pre = '';
                    var _aft = '';
                    if (canUseSelection) {
                        _pre = target['value'].substring(0, target[start]);
                        _aft = target['value'].substring(target[end], target['value'].length);
                    }
                    var text = (!canUseSelection) ? clipboard : _pre + clipboard + _aft;
                    if (target['maxLength'] && parseInt(target['maxLength'], 10) > 0) {
                        text = text.substr(0, parseInt(target['maxLength'], 10));
                    }
                    target['value'] = text;
                    target['focus']();
                }
            });
        }
    };
    AppComponent.prototype.contextMenuSelect = function (target) {
        if (target && (target['nodeName'].toUpperCase() === 'TEXTAREA' || target['nodeName'].toUpperCase() === 'INPUT')) {
            target['focus']();
            setTimeout(function () {
                target['select']();
            });
        }
    };
    AppComponent.prototype.ngOnDestroy = function () {
        if (this.intervalUpdateContractsState) {
            clearInterval(this.intervalUpdateContractsState);
        }
    };
    AppComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-root',
            template: __webpack_require__(/*! ./app.component.html */ "./src/app/app.component.html"),
            styles: [__webpack_require__(/*! ./app.component.scss */ "./src/app/app.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_common_http__WEBPACK_IMPORTED_MODULE_1__["HttpClient"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["Renderer2"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_2__["TranslateService"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _angular_router__WEBPACK_IMPORTED_MODULE_4__["Router"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_5__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], AppComponent);
    return AppComponent;
}());



/***/ }),

/***/ "./src/app/app.module.ts":
/*!*******************************!*\
  !*** ./src/app/app.module.ts ***!
  \*******************************/
/*! exports provided: HttpLoaderFactory, AppModule */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "HttpLoaderFactory", function() { return HttpLoaderFactory; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "AppModule", function() { return AppModule; });
/* harmony import */ var _angular_platform_browser__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/platform-browser */ "./node_modules/@angular/platform-browser/fesm5/platform-browser.js");
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _app_routing_module__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ./app-routing.module */ "./src/app/app-routing.module.ts");
/* harmony import */ var _app_component__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ./app.component */ "./src/app/app.component.ts");
/* harmony import */ var _login_login_component__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ./login/login.component */ "./src/app/login/login.component.ts");
/* harmony import */ var _settings_settings_component__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ./settings/settings.component */ "./src/app/settings/settings.component.ts");
/* harmony import */ var _sidebar_sidebar_component__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! ./sidebar/sidebar.component */ "./src/app/sidebar/sidebar.component.ts");
/* harmony import */ var _main_main_component__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! ./main/main.component */ "./src/app/main/main.component.ts");
/* harmony import */ var _create_wallet_create_wallet_component__WEBPACK_IMPORTED_MODULE_8__ = __webpack_require__(/*! ./create-wallet/create-wallet.component */ "./src/app/create-wallet/create-wallet.component.ts");
/* harmony import */ var _open_wallet_open_wallet_component__WEBPACK_IMPORTED_MODULE_9__ = __webpack_require__(/*! ./open-wallet/open-wallet.component */ "./src/app/open-wallet/open-wallet.component.ts");
/* harmony import */ var _restore_wallet_restore_wallet_component__WEBPACK_IMPORTED_MODULE_10__ = __webpack_require__(/*! ./restore-wallet/restore-wallet.component */ "./src/app/restore-wallet/restore-wallet.component.ts");
/* harmony import */ var _seed_phrase_seed_phrase_component__WEBPACK_IMPORTED_MODULE_11__ = __webpack_require__(/*! ./seed-phrase/seed-phrase.component */ "./src/app/seed-phrase/seed-phrase.component.ts");
/* harmony import */ var _wallet_details_wallet_details_component__WEBPACK_IMPORTED_MODULE_12__ = __webpack_require__(/*! ./wallet-details/wallet-details.component */ "./src/app/wallet-details/wallet-details.component.ts");
/* harmony import */ var _wallet_wallet_component__WEBPACK_IMPORTED_MODULE_13__ = __webpack_require__(/*! ./wallet/wallet.component */ "./src/app/wallet/wallet.component.ts");
/* harmony import */ var _send_send_component__WEBPACK_IMPORTED_MODULE_14__ = __webpack_require__(/*! ./send/send.component */ "./src/app/send/send.component.ts");
/* harmony import */ var _receive_receive_component__WEBPACK_IMPORTED_MODULE_15__ = __webpack_require__(/*! ./receive/receive.component */ "./src/app/receive/receive.component.ts");
/* harmony import */ var _history_history_component__WEBPACK_IMPORTED_MODULE_16__ = __webpack_require__(/*! ./history/history.component */ "./src/app/history/history.component.ts");
/* harmony import */ var _contracts_contracts_component__WEBPACK_IMPORTED_MODULE_17__ = __webpack_require__(/*! ./contracts/contracts.component */ "./src/app/contracts/contracts.component.ts");
/* harmony import */ var _purchase_purchase_component__WEBPACK_IMPORTED_MODULE_18__ = __webpack_require__(/*! ./purchase/purchase.component */ "./src/app/purchase/purchase.component.ts");
/* harmony import */ var _messages_messages_component__WEBPACK_IMPORTED_MODULE_19__ = __webpack_require__(/*! ./messages/messages.component */ "./src/app/messages/messages.component.ts");
/* harmony import */ var _staking_staking_component__WEBPACK_IMPORTED_MODULE_20__ = __webpack_require__(/*! ./staking/staking.component */ "./src/app/staking/staking.component.ts");
/* harmony import */ var _angular_common_http__WEBPACK_IMPORTED_MODULE_21__ = __webpack_require__(/*! @angular/common/http */ "./node_modules/@angular/common/fesm5/http.js");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_22__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
/* harmony import */ var _ngx_translate_http_loader__WEBPACK_IMPORTED_MODULE_23__ = __webpack_require__(/*! @ngx-translate/http-loader */ "./node_modules/@ngx-translate/http-loader/fesm5/ngx-translate-http-loader.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_24__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _typing_message_typing_message_component__WEBPACK_IMPORTED_MODULE_25__ = __webpack_require__(/*! ./typing-message/typing-message.component */ "./src/app/typing-message/typing-message.component.ts");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_26__ = __webpack_require__(/*! ./_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_27__ = __webpack_require__(/*! ./_helpers/pipes/money-to-int.pipe */ "./src/app/_helpers/pipes/money-to-int.pipe.ts");
/* harmony import */ var _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_28__ = __webpack_require__(/*! ./_helpers/pipes/int-to-money.pipe */ "./src/app/_helpers/pipes/int-to-money.pipe.ts");
/* harmony import */ var _helpers_directives_staking_switch_staking_switch_component__WEBPACK_IMPORTED_MODULE_29__ = __webpack_require__(/*! ./_helpers/directives/staking-switch/staking-switch.component */ "./src/app/_helpers/directives/staking-switch/staking-switch.component.ts");
/* harmony import */ var _helpers_directives_tooltip_directive__WEBPACK_IMPORTED_MODULE_30__ = __webpack_require__(/*! ./_helpers/directives/tooltip.directive */ "./src/app/_helpers/directives/tooltip.directive.ts");
/* harmony import */ var _helpers_pipes_history_type_messages_pipe__WEBPACK_IMPORTED_MODULE_31__ = __webpack_require__(/*! ./_helpers/pipes/history-type-messages.pipe */ "./src/app/_helpers/pipes/history-type-messages.pipe.ts");
/* harmony import */ var _helpers_pipes_contract_status_messages_pipe__WEBPACK_IMPORTED_MODULE_32__ = __webpack_require__(/*! ./_helpers/pipes/contract-status-messages.pipe */ "./src/app/_helpers/pipes/contract-status-messages.pipe.ts");
/* harmony import */ var _helpers_pipes_contract_time_left_pipe__WEBPACK_IMPORTED_MODULE_33__ = __webpack_require__(/*! ./_helpers/pipes/contract-time-left.pipe */ "./src/app/_helpers/pipes/contract-time-left.pipe.ts");
/* harmony import */ var ngx_contextmenu__WEBPACK_IMPORTED_MODULE_34__ = __webpack_require__(/*! ngx-contextmenu */ "./node_modules/ngx-contextmenu/fesm5/ngx-contextmenu.js");
/* harmony import */ var angular_highcharts__WEBPACK_IMPORTED_MODULE_35__ = __webpack_require__(/*! angular-highcharts */ "./node_modules/angular-highcharts/fesm5/angular-highcharts.js");
/* harmony import */ var _helpers_directives_input_validate_input_validate_directive__WEBPACK_IMPORTED_MODULE_36__ = __webpack_require__(/*! ./_helpers/directives/input-validate/input-validate.directive */ "./src/app/_helpers/directives/input-validate/input-validate.directive.ts");
/* harmony import */ var _helpers_directives_modal_container_modal_container_component__WEBPACK_IMPORTED_MODULE_37__ = __webpack_require__(/*! ./_helpers/directives/modal-container/modal-container.component */ "./src/app/_helpers/directives/modal-container/modal-container.component.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};



































function HttpLoaderFactory(httpClient) {
    return new _ngx_translate_http_loader__WEBPACK_IMPORTED_MODULE_23__["TranslateHttpLoader"](httpClient, './assets/i18n/', '.json');
}



// import * as more from 'highcharts/highcharts-more.src';
// import * as exporting from 'highcharts/modules/exporting.src';
// import * as highstock from 'highcharts/modules/stock.src';
angular_highcharts__WEBPACK_IMPORTED_MODULE_35__["Highcharts"].setOptions({
// global: {
//   useUTC: false
// }
});
var AppModule = /** @class */ (function () {
    function AppModule() {
    }
    AppModule = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_1__["NgModule"])({
            declarations: [
                _app_component__WEBPACK_IMPORTED_MODULE_3__["AppComponent"],
                _login_login_component__WEBPACK_IMPORTED_MODULE_4__["LoginComponent"],
                _settings_settings_component__WEBPACK_IMPORTED_MODULE_5__["SettingsComponent"],
                _sidebar_sidebar_component__WEBPACK_IMPORTED_MODULE_6__["SidebarComponent"],
                _main_main_component__WEBPACK_IMPORTED_MODULE_7__["MainComponent"],
                _create_wallet_create_wallet_component__WEBPACK_IMPORTED_MODULE_8__["CreateWalletComponent"],
                _open_wallet_open_wallet_component__WEBPACK_IMPORTED_MODULE_9__["OpenWalletComponent"],
                _restore_wallet_restore_wallet_component__WEBPACK_IMPORTED_MODULE_10__["RestoreWalletComponent"],
                _seed_phrase_seed_phrase_component__WEBPACK_IMPORTED_MODULE_11__["SeedPhraseComponent"],
                _wallet_details_wallet_details_component__WEBPACK_IMPORTED_MODULE_12__["WalletDetailsComponent"],
                _wallet_wallet_component__WEBPACK_IMPORTED_MODULE_13__["WalletComponent"],
                _send_send_component__WEBPACK_IMPORTED_MODULE_14__["SendComponent"],
                _receive_receive_component__WEBPACK_IMPORTED_MODULE_15__["ReceiveComponent"],
                _history_history_component__WEBPACK_IMPORTED_MODULE_16__["HistoryComponent"],
                _contracts_contracts_component__WEBPACK_IMPORTED_MODULE_17__["ContractsComponent"],
                _purchase_purchase_component__WEBPACK_IMPORTED_MODULE_18__["PurchaseComponent"],
                _messages_messages_component__WEBPACK_IMPORTED_MODULE_19__["MessagesComponent"],
                _staking_staking_component__WEBPACK_IMPORTED_MODULE_20__["StakingComponent"],
                _typing_message_typing_message_component__WEBPACK_IMPORTED_MODULE_25__["TypingMessageComponent"],
                _helpers_pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_27__["MoneyToIntPipe"],
                _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_28__["IntToMoneyPipe"],
                _helpers_directives_staking_switch_staking_switch_component__WEBPACK_IMPORTED_MODULE_29__["StakingSwitchComponent"],
                _helpers_pipes_history_type_messages_pipe__WEBPACK_IMPORTED_MODULE_31__["HistoryTypeMessagesPipe"],
                _helpers_pipes_contract_status_messages_pipe__WEBPACK_IMPORTED_MODULE_32__["ContractStatusMessagesPipe"],
                _helpers_pipes_contract_time_left_pipe__WEBPACK_IMPORTED_MODULE_33__["ContractTimeLeftPipe"],
                _helpers_directives_tooltip_directive__WEBPACK_IMPORTED_MODULE_30__["TooltipDirective"],
                _helpers_directives_input_validate_input_validate_directive__WEBPACK_IMPORTED_MODULE_36__["InputValidateDirective"],
                _helpers_directives_modal_container_modal_container_component__WEBPACK_IMPORTED_MODULE_37__["ModalContainerComponent"]
            ],
            imports: [
                _angular_platform_browser__WEBPACK_IMPORTED_MODULE_0__["BrowserModule"],
                _app_routing_module__WEBPACK_IMPORTED_MODULE_2__["AppRoutingModule"],
                _angular_common_http__WEBPACK_IMPORTED_MODULE_21__["HttpClientModule"],
                _ngx_translate_core__WEBPACK_IMPORTED_MODULE_22__["TranslateModule"].forRoot({
                    loader: {
                        provide: _ngx_translate_core__WEBPACK_IMPORTED_MODULE_22__["TranslateLoader"],
                        useFactory: HttpLoaderFactory,
                        deps: [_angular_common_http__WEBPACK_IMPORTED_MODULE_21__["HttpClient"]]
                    }
                }),
                _angular_forms__WEBPACK_IMPORTED_MODULE_24__["FormsModule"],
                _angular_forms__WEBPACK_IMPORTED_MODULE_24__["ReactiveFormsModule"],
                angular_highcharts__WEBPACK_IMPORTED_MODULE_35__["ChartModule"],
                ngx_contextmenu__WEBPACK_IMPORTED_MODULE_34__["ContextMenuModule"].forRoot()
            ],
            providers: [
                _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_26__["BackendService"],
                _helpers_pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_27__["MoneyToIntPipe"],
                _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_28__["IntToMoneyPipe"],
            ],
            bootstrap: [_app_component__WEBPACK_IMPORTED_MODULE_3__["AppComponent"]]
        })
    ], AppModule);
    return AppModule;
}());



/***/ }),

/***/ "./src/app/contracts/contracts.component.html":
/*!****************************************************!*\
  !*** ./src/app/contracts/contracts.component.html ***!
  \****************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"empty-contracts\" *ngIf=\"!variablesService.currentWallet.contracts.length\">\r\n  <span>{{ 'CONTRACTS.EMPTY' | translate }}</span>\r\n</div>\r\n\r\n<div class=\"wrap-table scrolled-content\" *ngIf=\"variablesService.currentWallet.contracts.length\">\r\n  <table class=\"contracts-table\">\r\n    <thead>\r\n    <tr>\r\n      <th>{{ 'CONTRACTS.CONTRACTS' | translate }}</th>\r\n      <th>{{ 'CONTRACTS.DATE' | translate }}</th>\r\n      <th>{{ 'CONTRACTS.AMOUNT' | translate }}</th>\r\n      <th>{{ 'CONTRACTS.STATUS' | translate }}</th>\r\n      <th>{{ 'CONTRACTS.COMMENTS' | translate }}</th>\r\n    </tr>\r\n    </thead>\r\n    <tbody>\r\n    <tr *ngFor=\"let item of variablesService.currentWallet.contracts\" [routerLink]=\"'/wallet/' + walletId + '/purchase/' + item.contract_id\">\r\n      <td>\r\n        <div class=\"contract\">\r\n          <i class=\"icon alert\" *ngIf=\"!item.is_new\"></i>\r\n          <i class=\"icon new\" *ngIf=\"item.is_new\"></i>\r\n          <i class=\"icon\" [class.purchase]=\"item.is_a\" [class.sell]=\"!item.is_a\"></i>\r\n          <span>{{item.private_detailes.t}}</span>\r\n        </div>\r\n      </td>\r\n      <td>{{item.timestamp * 1000 | date : 'dd-MM-yyyy HH:mm'}}</td>\r\n      <td>{{item.private_detailes.to_pay | intToMoney}} {{variablesService.defaultCurrency}}</td>\r\n      <td>\r\n        <div class=\"status\">\r\n          {{item | contractStatusMessages}}\r\n        </div>\r\n      </td>\r\n      <td>\r\n        <div class=\"comment\">\r\n          {{item.private_detailes.c}}\r\n        </div>\r\n      </td>\r\n    </tr>\r\n    </tbody>\r\n  </table>\r\n</div>\r\n\r\n<div class=\"contracts-buttons\">\r\n  <button type=\"button\" class=\"blue-button\" [routerLink]=\"'/wallet/' + walletId + '/purchase'\">{{ 'CONTRACTS.PURCHASE_BUTTON' | translate }}</button>\r\n  <button type=\"button\" class=\"blue-button\" disabled>{{ 'CONTRACTS.LISTING_BUTTON' | translate }}</button>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/contracts/contracts.component.scss":
/*!****************************************************!*\
  !*** ./src/app/contracts/contracts.component.scss ***!
  \****************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.empty-contracts {\n  font-size: 1.5rem; }\n\n.wrap-table {\n  margin: -3rem -3rem 0 -3rem;\n  overflow-x: auto; }\n\n.wrap-table .contract {\n    position: relative;\n    display: flex;\n    align-items: center; }\n\n.wrap-table .contract .icon {\n      flex-shrink: 0; }\n\n.wrap-table .contract .icon.new, .wrap-table .contract .icon.alert {\n        position: absolute;\n        top: 0; }\n\n.wrap-table .contract .icon.new {\n        left: -2.3rem;\n        -webkit-mask: url('new.svg') no-repeat center;\n                mask: url('new.svg') no-repeat center;\n        width: 1.7rem;\n        height: 1.7rem; }\n\n.wrap-table .contract .icon.alert {\n        top: 0.2rem;\n        left: -2.1rem;\n        -webkit-mask: url('alert.svg') no-repeat center;\n                mask: url('alert.svg') no-repeat center;\n        width: 1.2rem;\n        height: 1.2rem; }\n\n.wrap-table .contract .icon.purchase, .wrap-table .contract .icon.sell {\n        margin-right: 1rem;\n        width: 1.5rem;\n        height: 1.5rem; }\n\n.wrap-table .contract .icon.purchase {\n        -webkit-mask: url('purchase.svg') no-repeat center;\n                mask: url('purchase.svg') no-repeat center; }\n\n.wrap-table .contract .icon.sell {\n        -webkit-mask: url('sell.svg') no-repeat center;\n                mask: url('sell.svg') no-repeat center; }\n\n.wrap-table .contract span {\n      text-overflow: ellipsis;\n      overflow: hidden;\n      max-width: 20vw; }\n\n.wrap-table .status, .wrap-table .comment {\n    text-overflow: ellipsis;\n    overflow: hidden;\n    max-width: 20vw; }\n\n.contracts-buttons {\n  display: flex;\n  margin: 3rem 0;\n  width: 50%; }\n\n.contracts-buttons button {\n    flex: 0 1 50%;\n    margin-right: 1.5rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvY29udHJhY3RzL0Q6XFx6YW5vL3NyY1xcYXBwXFxjb250cmFjdHNcXGNvbnRyYWN0cy5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLFlBQVcsRUFDWjs7QUFFRDtFQUNFLGtCQUFpQixFQUNsQjs7QUFFRDtFQUNFLDRCQUEyQjtFQUMzQixpQkFBZ0IsRUF5RGpCOztBQTNERDtJQUtJLG1CQUFrQjtJQUNsQixjQUFhO0lBQ2Isb0JBQW1CLEVBNkNwQjs7QUFwREg7TUFVTSxlQUFjLEVBbUNmOztBQTdDTDtRQWFRLG1CQUFrQjtRQUNsQixPQUFNLEVBQ1A7O0FBZlA7UUFrQlEsY0FBYTtRQUNiLDhDQUFzRDtnQkFBdEQsc0NBQXNEO1FBQ3RELGNBQWE7UUFDYixlQUFjLEVBQ2Y7O0FBdEJQO1FBeUJRLFlBQVc7UUFDWCxjQUFhO1FBQ2IsZ0RBQXdEO2dCQUF4RCx3Q0FBd0Q7UUFDeEQsY0FBYTtRQUNiLGVBQWMsRUFDZjs7QUE5QlA7UUFpQ1EsbUJBQWtCO1FBQ2xCLGNBQWE7UUFDYixlQUFjLEVBQ2Y7O0FBcENQO1FBdUNRLG1EQUEyRDtnQkFBM0QsMkNBQTJELEVBQzVEOztBQXhDUDtRQTJDUSwrQ0FBdUQ7Z0JBQXZELHVDQUF1RCxFQUN4RDs7QUE1Q1A7TUFnRE0sd0JBQXVCO01BQ3ZCLGlCQUFnQjtNQUNoQixnQkFBZSxFQUNoQjs7QUFuREw7SUF1REksd0JBQXVCO0lBQ3ZCLGlCQUFnQjtJQUNoQixnQkFBZSxFQUNoQjs7QUFHSDtFQUNFLGNBQWE7RUFDYixlQUFjO0VBQ2QsV0FBVSxFQU1YOztBQVREO0lBTUksY0FBYTtJQUNiLHFCQUFvQixFQUNyQiIsImZpbGUiOiJzcmMvYXBwL2NvbnRyYWN0cy9jb250cmFjdHMuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XHJcbiAgd2lkdGg6IDEwMCU7XHJcbn1cclxuXHJcbi5lbXB0eS1jb250cmFjdHMge1xyXG4gIGZvbnQtc2l6ZTogMS41cmVtO1xyXG59XHJcblxyXG4ud3JhcC10YWJsZSB7XHJcbiAgbWFyZ2luOiAtM3JlbSAtM3JlbSAwIC0zcmVtO1xyXG4gIG92ZXJmbG93LXg6IGF1dG87XHJcblxyXG4gIC5jb250cmFjdCB7XHJcbiAgICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuXHJcbiAgICAuaWNvbiB7XHJcbiAgICAgIGZsZXgtc2hyaW5rOiAwO1xyXG5cclxuICAgICAgJi5uZXcsICYuYWxlcnQge1xyXG4gICAgICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuICAgICAgICB0b3A6IDA7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYubmV3IHtcclxuICAgICAgICBsZWZ0OiAtMi4zcmVtO1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvbmV3LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICB3aWR0aDogMS43cmVtO1xyXG4gICAgICAgIGhlaWdodDogMS43cmVtO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLmFsZXJ0IHtcclxuICAgICAgICB0b3A6IDAuMnJlbTtcclxuICAgICAgICBsZWZ0OiAtMi4xcmVtO1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvYWxlcnQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgIHdpZHRoOiAxLjJyZW07XHJcbiAgICAgICAgaGVpZ2h0OiAxLjJyZW07XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYucHVyY2hhc2UsICYuc2VsbCB7XHJcbiAgICAgICAgbWFyZ2luLXJpZ2h0OiAxcmVtO1xyXG4gICAgICAgIHdpZHRoOiAxLjVyZW07XHJcbiAgICAgICAgaGVpZ2h0OiAxLjVyZW07XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYucHVyY2hhc2Uge1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvcHVyY2hhc2Uuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLnNlbGwge1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvc2VsbC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIH1cclxuICAgIH1cclxuXHJcbiAgICBzcGFuIHtcclxuICAgICAgdGV4dC1vdmVyZmxvdzogZWxsaXBzaXM7XHJcbiAgICAgIG92ZXJmbG93OiBoaWRkZW47XHJcbiAgICAgIG1heC13aWR0aDogMjB2dztcclxuICAgIH1cclxuICB9XHJcblxyXG4gIC5zdGF0dXMsIC5jb21tZW50IHtcclxuICAgIHRleHQtb3ZlcmZsb3c6IGVsbGlwc2lzO1xyXG4gICAgb3ZlcmZsb3c6IGhpZGRlbjtcclxuICAgIG1heC13aWR0aDogMjB2dztcclxuICB9XHJcbn1cclxuXHJcbi5jb250cmFjdHMtYnV0dG9ucyB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBtYXJnaW46IDNyZW0gMDtcclxuICB3aWR0aDogNTAlO1xyXG5cclxuICBidXR0b24ge1xyXG4gICAgZmxleDogMCAxIDUwJTtcclxuICAgIG1hcmdpbi1yaWdodDogMS41cmVtO1xyXG4gIH1cclxufVxyXG4iXX0= */"

/***/ }),

/***/ "./src/app/contracts/contracts.component.ts":
/*!**************************************************!*\
  !*** ./src/app/contracts/contracts.component.ts ***!
  \**************************************************/
/*! exports provided: ContractsComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ContractsComponent", function() { return ContractsComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};



var ContractsComponent = /** @class */ (function () {
    function ContractsComponent(route, router, variablesService) {
        this.route = route;
        this.router = router;
        this.variablesService = variablesService;
    }
    ContractsComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function (params) {
            if (params.hasOwnProperty('id')) {
                _this.walletId = params['id'];
            }
        });
    };
    ContractsComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
    };
    ContractsComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-contracts',
            template: __webpack_require__(/*! ./contracts.component.html */ "./src/app/contracts/contracts.component.html"),
            styles: [__webpack_require__(/*! ./contracts.component.scss */ "./src/app/contracts/contracts.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_1__["ActivatedRoute"],
            _angular_router__WEBPACK_IMPORTED_MODULE_1__["Router"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"]])
    ], ContractsComponent);
    return ContractsComponent;
}());



/***/ }),

/***/ "./src/app/create-wallet/create-wallet.component.html":
/*!************************************************************!*\
  !*** ./src/app/create-wallet/create-wallet.component.html ***!
  \************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span>{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\r\n      <span>{{ 'BREADCRUMBS.CREATE_WALLET' | translate }}</span>\r\n    </div>\r\n    <a class=\"back-btn\" [routerLink]=\"['/main']\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </a>\r\n  </div>\r\n\r\n  <form class=\"form-create\" [formGroup]=\"createForm\">\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-name\">{{ 'CREATE_WALLET.NAME' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [attr.disabled]=\"walletSaved ? '' : null\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"createForm.controls['name'].invalid && (createForm.controls['name'].dirty || createForm.controls['name'].touched)\">\r\n      <div *ngIf=\"createForm.controls['name'].errors['required']\">\r\n        Name is required.\r\n      </div>\r\n      <div *ngIf=\"createForm.controls['name'].errors['duplicate']\">\r\n        Name is duplicate.\r\n      </div>\r\n    </div>\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-password\">{{ 'CREATE_WALLET.PASS' | translate }}</label>\r\n      <input type=\"password\" id=\"wallet-password\" formControlName=\"password\" [attr.disabled]=\"walletSaved ? '' : null\">\r\n    </div>\r\n    <div class=\"input-block\">\r\n      <label for=\"confirm-wallet-password\">{{ 'CREATE_WALLET.CONFIRM' | translate }}</label>\r\n      <input type=\"password\" id=\"confirm-wallet-password\" formControlName=\"confirm\" [attr.disabled]=\"walletSaved ? '' : null\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"createForm.controls['password'].dirty && createForm.controls['confirm'].dirty && createForm.errors\">\r\n      <div *ngIf=\"createForm.errors['confirm_mismatch']\">\r\n        Confirm password not match.\r\n      </div>\r\n    </div>\r\n    <div class=\"wrap-buttons\">\r\n      <button type=\"button\" class=\"transparent-button\" *ngIf=\"walletSaved\" disabled><i class=\"icon\"></i>{{createForm.controls['name'].value}}</button>\r\n      <button type=\"button\" class=\"blue-button select-button\" (click)=\"saveWallet()\" [disabled]=\"!createForm.valid\" *ngIf=\"!walletSaved\">{{ 'CREATE_WALLET.BUTTON_SELECT' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button create-button\" (click)=\"createWallet()\" [disabled]=\"!walletSaved\">{{ 'CREATE_WALLET.BUTTON_CREATE' | translate }}</button>\r\n    </div>\r\n  </form>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/create-wallet/create-wallet.component.scss":
/*!************************************************************!*\
  !*** ./src/app/create-wallet/create-wallet.component.scss ***!
  \************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-create {\n  margin: 2.4rem 0;\n  width: 50%; }\n  .form-create .wrap-buttons {\n    display: flex;\n    margin: 2.5rem -0.7rem; }\n  .form-create .wrap-buttons button {\n      margin: 0 0.7rem; }\n  .form-create .wrap-buttons button.transparent-button {\n        flex-basis: 50%; }\n  .form-create .wrap-buttons button.select-button {\n        flex-basis: 60%; }\n  .form-create .wrap-buttons button.create-button {\n        flex: 1 1 50%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvY3JlYXRlLXdhbGxldC9EOlxcemFuby9zcmNcXGFwcFxcY3JlYXRlLXdhbGxldFxcY3JlYXRlLXdhbGxldC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGlCQUFnQjtFQUNoQixXQUFVLEVBc0JYO0VBeEJEO0lBS0ksY0FBYTtJQUNiLHVCQUFzQixFQWlCdkI7RUF2Qkg7TUFTTSxpQkFBZ0IsRUFhakI7RUF0Qkw7UUFZUSxnQkFBZSxFQUNoQjtFQWJQO1FBZ0JRLGdCQUFlLEVBQ2hCO0VBakJQO1FBb0JRLGNBQWEsRUFDZCIsImZpbGUiOiJzcmMvYXBwL2NyZWF0ZS13YWxsZXQvY3JlYXRlLXdhbGxldC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIi5mb3JtLWNyZWF0ZSB7XHJcbiAgbWFyZ2luOiAyLjRyZW0gMDtcclxuICB3aWR0aDogNTAlO1xyXG5cclxuICAud3JhcC1idXR0b25zIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBtYXJnaW46IDIuNXJlbSAtMC43cmVtO1xyXG5cclxuICAgIGJ1dHRvbiB7XHJcbiAgICAgIG1hcmdpbjogMCAwLjdyZW07XHJcblxyXG4gICAgICAmLnRyYW5zcGFyZW50LWJ1dHRvbiB7XHJcbiAgICAgICAgZmxleC1iYXNpczogNTAlO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLnNlbGVjdC1idXR0b24ge1xyXG4gICAgICAgIGZsZXgtYmFzaXM6IDYwJTtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi5jcmVhdGUtYnV0dG9uIHtcclxuICAgICAgICBmbGV4OiAxIDEgNTAlO1xyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcbiJdfQ== */"

/***/ }),

/***/ "./src/app/create-wallet/create-wallet.component.ts":
/*!**********************************************************!*\
  !*** ./src/app/create-wallet/create-wallet.component.ts ***!
  \**********************************************************/
/*! exports provided: CreateWalletComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "CreateWalletComponent", function() { return CreateWalletComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/models/wallet.model */ "./src/app/_helpers/models/wallet.model.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var CreateWalletComponent = /** @class */ (function () {
    function CreateWalletComponent(router, backend, variablesService, ngZone) {
        var _this = this;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.createForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            name: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [_angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required, function (g) {
                    for (var i = 0; i < _this.variablesService.wallets.length; i++) {
                        if (g.value === _this.variablesService.wallets[i].name) {
                            return { 'duplicate': true };
                        }
                    }
                    return null;
                }]),
            password: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](''),
            confirm: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('')
        }, function (g) {
            return g.get('password').value === g.get('confirm').value ? null : { 'confirm_mismatch': true };
        });
        this.wallet = {
            id: ''
        };
        this.walletSaved = false;
    }
    CreateWalletComponent.prototype.ngOnInit = function () {
    };
    CreateWalletComponent.prototype.createWallet = function () {
        this.router.navigate(['/seed-phrase'], { queryParams: { wallet_id: this.wallet.id } });
    };
    CreateWalletComponent.prototype.saveWallet = function () {
        var _this = this;
        if (this.createForm.valid) {
            this.backend.saveFileDialog('Save the wallet file.', '*', this.variablesService.settings.default_path, function (file_status, file_data) {
                if (file_status) {
                    _this.variablesService.settings.default_path = file_data.path.substr(0, file_data.path.lastIndexOf('/'));
                    _this.backend.generateWallet(file_data.path, _this.createForm.get('password').value, function (generate_status, generate_data, errorCode) {
                        if (generate_status) {
                            _this.wallet.id = generate_data.wallet_id;
                            _this.variablesService.opening_wallet = new _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_5__["Wallet"](generate_data.wallet_id, _this.createForm.get('name').value, _this.createForm.get('password').value, generate_data['wi'].path, generate_data['wi'].address, generate_data['wi'].balance, generate_data['wi'].unlocked_balance, generate_data['wi'].mined_total, generate_data['wi'].tracking_hey);
                            _this.ngZone.run(function () {
                                _this.walletSaved = true;
                            });
                        }
                        else {
                            if (errorCode && errorCode === 'ALREADY_EXISTS') {
                                alert('You cannot record a file on top of another file');
                            }
                            else {
                                alert('You cannot save a safe file to the system partition');
                            }
                        }
                    });
                }
            });
        }
    };
    CreateWalletComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-create-wallet',
            template: __webpack_require__(/*! ./create-wallet.component.html */ "./src/app/create-wallet/create-wallet.component.html"),
            styles: [__webpack_require__(/*! ./create-wallet.component.scss */ "./src/app/create-wallet/create-wallet.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_4__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], CreateWalletComponent);
    return CreateWalletComponent;
}());



/***/ }),

/***/ "./src/app/history/history.component.html":
/*!************************************************!*\
  !*** ./src/app/history/history.component.html ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"wrap-table\">\r\n  <table class=\"history-table\">\r\n    <thead>\r\n    <tr>\r\n      <th>{{ 'HISTORY.STATUS' | translate }}</th>\r\n      <th>{{ 'HISTORY.DATE' | translate }}</th>\r\n      <th>{{ 'HISTORY.AMOUNT' | translate }}</th>\r\n      <th>{{ 'HISTORY.FEE' | translate }}</th>\r\n      <th>{{ 'HISTORY.ADDRESS' | translate }}</th>\r\n    </tr>\r\n    </thead>\r\n    <tbody>\r\n    <tr *ngFor=\"let item of variablesService.currentWallet.history\">\r\n      <td>\r\n        <div class=\"status\" [class.send]=\"!item.is_income\" [class.received]=\"item.is_income\">\r\n          <div class=\"confirmation\" tooltip=\"{{ 'HISTORY.STATUS_TOOLTIP' | translate : {'current': getHeight(item)/10, 'total': 10} }}\" placement=\"bottom\" tooltipClass=\"history-tooltip\" delay=\"500\">\r\n            <div class=\"fill\" [style.height]=\"getHeight(item) + '%'\"></div>\r\n          </div>\r\n          <i class=\"icon\"></i>\r\n          <span>{{ (item.is_income ? 'HISTORY.RECEIVED' : 'HISTORY.SEND') | translate }}</span>\r\n        </div>\r\n      </td>\r\n      <td>{{item.timestamp * 1000 | date : 'dd-MM-yyyy HH:mm'}}</td>\r\n      <td>{{item.sortAmount | intToMoney}} {{variablesService.defaultCurrency}}</td>\r\n      <td>\r\n        <span *ngIf=\"item.sortFee\">{{item.sortFee | intToMoney}} {{variablesService.defaultCurrency}}</span>\r\n      </td>\r\n      <td class=\"remote-address\">\r\n        <span>{{item | historyTypeMessages}}</span>\r\n      </td>\r\n    </tr>\r\n    </tbody>\r\n  </table>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/history/history.component.scss":
/*!************************************************!*\
  !*** ./src/app/history/history.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.wrap-table {\n  margin: -3rem; }\n\n.wrap-table table tbody tr .status {\n    position: relative;\n    display: flex;\n    align-items: center; }\n\n.wrap-table table tbody tr .status .confirmation {\n      position: absolute;\n      top: 50%;\n      left: -2rem;\n      -webkit-transform: translateY(-50%);\n              transform: translateY(-50%);\n      display: flex;\n      align-items: flex-end;\n      width: 0.7rem;\n      height: 1.5rem; }\n\n.wrap-table table tbody tr .status .confirmation .fill {\n        width: 100%; }\n\n.wrap-table table tbody tr .status .icon {\n      margin-right: 1rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.wrap-table table tbody tr .status.send .icon {\n      -webkit-mask: url('send.svg') no-repeat center;\n              mask: url('send.svg') no-repeat center; }\n\n.wrap-table table tbody tr .status.received .icon {\n      -webkit-mask: url('receive.svg') no-repeat center;\n              mask: url('receive.svg') no-repeat center; }\n\n.wrap-table table tbody tr .remote-address {\n    overflow: hidden;\n    text-overflow: ellipsis;\n    max-width: 25vw; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvaGlzdG9yeS9EOlxcemFuby9zcmNcXGFwcFxcaGlzdG9yeVxcaGlzdG9yeS5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLFlBQVcsRUFDWjs7QUFFRDtFQUNFLGNBQWEsRUF5RGQ7O0FBMUREO0lBVVUsbUJBQWtCO0lBQ2xCLGNBQWE7SUFDYixvQkFBbUIsRUFvQ3BCOztBQWhEVDtNQWVZLG1CQUFrQjtNQUNsQixTQUFRO01BQ1IsWUFBVztNQUNYLG9DQUEyQjtjQUEzQiw0QkFBMkI7TUFDM0IsY0FBYTtNQUNiLHNCQUFxQjtNQUNyQixjQUFhO01BQ2IsZUFBYyxFQUtmOztBQTNCWDtRQXlCYyxZQUFXLEVBQ1o7O0FBMUJiO01BOEJZLG1CQUFrQjtNQUNsQixjQUFhO01BQ2IsZUFBYyxFQUNmOztBQWpDWDtNQXNDYywrQ0FBdUQ7Y0FBdkQsdUNBQXVELEVBQ3hEOztBQXZDYjtNQTZDYyxrREFBMEQ7Y0FBMUQsMENBQTBELEVBQzNEOztBQTlDYjtJQW1EVSxpQkFBZ0I7SUFDaEIsd0JBQXVCO0lBQ3ZCLGdCQUFlLEVBQ2hCIiwiZmlsZSI6InNyYy9hcHAvaGlzdG9yeS9oaXN0b3J5LmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xyXG4gIHdpZHRoOiAxMDAlO1xyXG59XHJcblxyXG4ud3JhcC10YWJsZSB7XHJcbiAgbWFyZ2luOiAtM3JlbTtcclxuXHJcbiAgdGFibGUge1xyXG5cclxuICAgIHRib2R5IHtcclxuXHJcbiAgICAgIHRyIHtcclxuXHJcbiAgICAgICAgLnN0YXR1cyB7XHJcbiAgICAgICAgICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgICAgICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuXHJcbiAgICAgICAgICAuY29uZmlybWF0aW9uIHtcclxuICAgICAgICAgICAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gICAgICAgICAgICB0b3A6IDUwJTtcclxuICAgICAgICAgICAgbGVmdDogLTJyZW07XHJcbiAgICAgICAgICAgIHRyYW5zZm9ybTogdHJhbnNsYXRlWSgtNTAlKTtcclxuICAgICAgICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgICAgICAgYWxpZ24taXRlbXM6IGZsZXgtZW5kO1xyXG4gICAgICAgICAgICB3aWR0aDogMC43cmVtO1xyXG4gICAgICAgICAgICBoZWlnaHQ6IDEuNXJlbTtcclxuXHJcbiAgICAgICAgICAgIC5maWxsIHtcclxuICAgICAgICAgICAgICB3aWR0aDogMTAwJTtcclxuICAgICAgICAgICAgfVxyXG4gICAgICAgICAgfVxyXG5cclxuICAgICAgICAgIC5pY29uIHtcclxuICAgICAgICAgICAgbWFyZ2luLXJpZ2h0OiAxcmVtO1xyXG4gICAgICAgICAgICB3aWR0aDogMS43cmVtO1xyXG4gICAgICAgICAgICBoZWlnaHQ6IDEuN3JlbTtcclxuICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAmLnNlbmQgIHtcclxuXHJcbiAgICAgICAgICAgIC5pY29uIHtcclxuICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3NlbmQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgICAgICB9XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgJi5yZWNlaXZlZCB7XHJcblxyXG4gICAgICAgICAgICAuaWNvbiB7XHJcbiAgICAgICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9yZWNlaXZlLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICAgICAgfVxyXG4gICAgICAgICAgfVxyXG4gICAgICAgIH1cclxuXHJcbiAgICAgICAgLnJlbW90ZS1hZGRyZXNzIHtcclxuICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XHJcbiAgICAgICAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcclxuICAgICAgICAgIG1heC13aWR0aDogMjV2dztcclxuICAgICAgICB9XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcbn1cclxuIl19 */"

/***/ }),

/***/ "./src/app/history/history.component.ts":
/*!**********************************************!*\
  !*** ./src/app/history/history.component.ts ***!
  \**********************************************/
/*! exports provided: HistoryComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "HistoryComponent", function() { return HistoryComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};




var HistoryComponent = /** @class */ (function () {
    function HistoryComponent(route, backend, variablesService) {
        this.route = route;
        this.backend = backend;
        this.variablesService = variablesService;
    }
    HistoryComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function () {
            console.log(_this.variablesService.currentWallet.history);
        });
    };
    HistoryComponent.prototype.getHeight = function (item) {
        if ((this.variablesService.height_app - item.height >= 10 && item.height !== 0) || (item.is_mining === true && item.height === 0)) {
            return 100;
        }
        else {
            if (item.height === 0 || this.variablesService.height_app - item.height < 0) {
                return 0;
            }
            else {
                return (this.variablesService.height_app - item.height) * 10;
            }
        }
    };
    HistoryComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
    };
    HistoryComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-history',
            template: __webpack_require__(/*! ./history.component.html */ "./src/app/history/history.component.html"),
            styles: [__webpack_require__(/*! ./history.component.scss */ "./src/app/history/history.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_1__["ActivatedRoute"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"]])
    ], HistoryComponent);
    return HistoryComponent;
}());



/***/ }),

/***/ "./src/app/login/login.component.html":
/*!********************************************!*\
  !*** ./src/app/login/login.component.html ***!
  \********************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\r\n  <div class=\"wrap-login\">\r\n\r\n    <div class=\"logo\"></div>\r\n\r\n    <form *ngIf=\"type === 'reg'\" class=\"form-login\" [formGroup]=\"regForm\" (ngSubmit)=\"onSubmitCreatePass()\">\r\n      <div class=\"input-block\">\r\n        <label for=\"master-pass\">{{ 'LOGIN.SETUP_MASTER_PASS' | translate }}</label>\r\n        <input type=\"password\" id=\"master-pass\" formControlName=\"password\">\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"regForm.controls['password'].invalid && (regForm.controls['password'].dirty || regForm.controls['password'].touched)\">\r\n        <div *ngIf=\"regForm.controls['password'].errors['required']\">\r\n          Password is required.\r\n        </div>\r\n      </div>\r\n      <div class=\"input-block\">\r\n        <label for=\"confirm-pass\">{{ 'LOGIN.SETUP_CONFIRM_PASS' | translate }}</label>\r\n        <input type=\"password\" id=\"confirm-pass\" formControlName=\"confirmation\">\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"regForm.controls['confirmation'].invalid && (regForm.controls['confirmation'].dirty || regForm.controls['confirmation'].touched)\">\r\n        <div *ngIf=\"regForm.controls['confirmation'].errors['required']\">\r\n          Confirmation is required.\r\n        </div>\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"regForm.controls['password'].dirty && regForm.controls['confirmation'].dirty && regForm.errors\">\r\n        <div *ngIf=\"regForm.errors['mismatch']\">\r\n          Mismatch.\r\n        </div>\r\n      </div>\r\n      <button type=\"submit\" class=\"blue-button\">{{ 'LOGIN.BUTTON_NEXT' | translate }}</button>\r\n    </form>\r\n\r\n    <form *ngIf=\"type !== 'reg'\" class=\"form-login\" [formGroup]=\"authForm\" (ngSubmit)=\"onSubmitAuthPass()\">\r\n      <div class=\"input-block\">\r\n        <label for=\"master-pass-login\">{{ 'LOGIN.MASTER_PASS' | translate }}</label>\r\n        <input type=\"password\" id=\"master-pass-login\" formControlName=\"password\" autofocus>\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"authForm.controls['password'].invalid && (authForm.controls['password'].dirty || authForm.controls['password'].touched)\">\r\n        <div *ngIf=\"authForm.controls['password'].errors['required']\">\r\n          Password is required.\r\n        </div>\r\n      </div>\r\n      <button type=\"submit\" class=\"blue-button\">{{ 'LOGIN.BUTTON_NEXT' | translate }}</button>\r\n    </form>\r\n\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/login/login.component.scss":
/*!********************************************!*\
  !*** ./src/app/login/login.component.scss ***!
  \********************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: fixed;\n  top: 0;\n  left: 0;\n  width: 100%;\n  height: 100%; }\n  :host .content {\n    display: flex; }\n  :host .content .wrap-login {\n      margin: auto;\n      width: 100%;\n      max-width: 40rem; }\n  :host .content .wrap-login .logo {\n        background: url('logo.png') no-repeat center top;\n        width: 100%;\n        height: 14rem; }\n  :host .content .wrap-login .form-login {\n        display: flex;\n        flex-direction: column; }\n  :host .content .wrap-login .form-login button {\n          margin: 2.5rem auto;\n          width: 100%;\n          max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvbG9naW4vRDpcXHphbm8vc3JjXFxhcHBcXGxvZ2luXFxsb2dpbi5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGdCQUFlO0VBQ2YsT0FBTTtFQUNOLFFBQU87RUFDUCxZQUFXO0VBQ1gsYUFBWSxFQTRCYjtFQWpDRDtJQVFJLGNBQWEsRUF3QmQ7RUFoQ0g7TUFXTSxhQUFZO01BQ1osWUFBVztNQUNYLGlCQUFnQixFQWtCakI7RUEvQkw7UUFnQlEsaURBQW9FO1FBQ3BFLFlBQVc7UUFDWCxjQUFhLEVBQ2Q7RUFuQlA7UUFzQlEsY0FBYTtRQUNiLHVCQUFzQixFQU92QjtFQTlCUDtVQTBCVSxvQkFBbUI7VUFDbkIsWUFBVztVQUNYLGlCQUFnQixFQUNqQiIsImZpbGUiOiJzcmMvYXBwL2xvZ2luL2xvZ2luLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xyXG4gIHBvc2l0aW9uOiBmaXhlZDtcclxuICB0b3A6IDA7XHJcbiAgbGVmdDogMDtcclxuICB3aWR0aDogMTAwJTtcclxuICBoZWlnaHQ6IDEwMCU7XHJcblxyXG4gIC5jb250ZW50IHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcblxyXG4gICAgLndyYXAtbG9naW4ge1xyXG4gICAgICBtYXJnaW46IGF1dG87XHJcbiAgICAgIHdpZHRoOiAxMDAlO1xyXG4gICAgICBtYXgtd2lkdGg6IDQwcmVtO1xyXG5cclxuICAgICAgLmxvZ28ge1xyXG4gICAgICAgIGJhY2tncm91bmQ6IHVybChcIi4uLy4uL2Fzc2V0cy9pbWFnZXMvbG9nby5wbmdcIikgbm8tcmVwZWF0IGNlbnRlciB0b3A7XHJcbiAgICAgICAgd2lkdGg6IDEwMCU7XHJcbiAgICAgICAgaGVpZ2h0OiAxNHJlbTtcclxuICAgICAgfVxyXG5cclxuICAgICAgLmZvcm0tbG9naW4ge1xyXG4gICAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuXHJcbiAgICAgICAgYnV0dG9uIHtcclxuICAgICAgICAgIG1hcmdpbjogMi41cmVtIGF1dG87XHJcbiAgICAgICAgICB3aWR0aDogMTAwJTtcclxuICAgICAgICAgIG1heC13aWR0aDogMTVyZW07XHJcbiAgICAgICAgfVxyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcbiJdfQ== */"

/***/ }),

/***/ "./src/app/login/login.component.ts":
/*!******************************************!*\
  !*** ./src/app/login/login.component.ts ***!
  \******************************************/
/*! exports provided: LoginComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "LoginComponent", function() { return LoginComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/models/wallet.model */ "./src/app/_helpers/models/wallet.model.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var LoginComponent = /** @class */ (function () {
    function LoginComponent(route, router, backend, variablesService, ngZone) {
        this.route = route;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.regForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            password: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required),
            confirmation: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required)
        }, function (g) {
            return g.get('password').value === g.get('confirmation').value ? null : { 'mismatch': true };
        });
        this.authForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            password: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required)
        });
        this.type = 'reg';
    }
    LoginComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.route.queryParams.subscribe(function (params) {
            if (params.type) {
                _this.type = params.type;
            }
        });
    };
    LoginComponent.prototype.onSubmitCreatePass = function () {
        var _this = this;
        if (this.regForm.valid) {
            this.variablesService.appPass = this.regForm.get('password').value;
            this.backend.storeSecureAppData(function (status, data) {
                if (status) {
                    _this.ngZone.run(function () {
                        _this.router.navigate(['/']);
                    });
                }
                else {
                    // TODO error sign in
                    console.log(data['error_code']);
                }
            });
        }
    };
    LoginComponent.prototype.onSubmitAuthPass = function () {
        var _this = this;
        if (this.authForm.valid) {
            var appPass_1 = this.authForm.get('password').value;
            this.backend.getSecureAppData({ pass: appPass_1 }, function (status, data) {
                if (data.error_code && data.error_code === 'WRONG_PASSWORD') {
                    // TODO error log in informer.error('MESSAGE.INCORRECT_PASSWORD');
                    console.log('WRONG_PASSWORD');
                }
                else {
                    _this.variablesService.startCountdown();
                    _this.variablesService.appPass = appPass_1;
                    if (_this.variablesService.wallets.length) {
                        _this.ngZone.run(function () {
                            _this.router.navigate(['/wallet/' + _this.variablesService.wallets[0].wallet_id]);
                        });
                        return;
                    }
                    if (Object.keys(data).length !== 0) {
                        var openWallets_1 = 0;
                        var runWallets_1 = 0;
                        data.forEach(function (wallet, wallet_index) {
                            _this.backend.openWallet(wallet.path, wallet.pass, true, function (open_status, open_data) {
                                if (open_status) {
                                    openWallets_1++;
                                    _this.backend.runWallet(open_data.wallet_id, function (run_status) {
                                        if (run_status) {
                                            runWallets_1++;
                                            _this.ngZone.run(function () {
                                                var new_wallet = new _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_5__["Wallet"](open_data.wallet_id, wallet.name, wallet.pass, open_data['wi'].path, open_data['wi'].address, open_data['wi'].balance, open_data['wi'].unlocked_balance, open_data['wi'].mined_total, open_data['wi'].tracking_hey);
                                                if (open_data.recent_history && open_data.recent_history.history) {
                                                    new_wallet.prepareHistory(open_data.recent_history.history);
                                                }
                                                _this.backend.getContracts(open_data.wallet_id, function (contracts_status, contracts_data) {
                                                    if (contracts_status && contracts_data.hasOwnProperty('contracts')) {
                                                        _this.ngZone.run(function () {
                                                            new_wallet.prepareContractsAfterOpen(contracts_data.contracts, _this.variablesService.exp_med_ts, _this.variablesService.height_app, _this.variablesService.settings.viewedContracts, _this.variablesService.settings.notViewedContracts);
                                                        });
                                                    }
                                                });
                                                _this.variablesService.wallets.push(new_wallet);
                                                if (_this.variablesService.wallets.length === 1) {
                                                    _this.router.navigate(['/wallet/' + _this.variablesService.wallets[0].wallet_id]);
                                                }
                                            });
                                        }
                                        else {
                                            if (wallet_index === data.length - 1 && runWallets_1 === 0) {
                                                _this.ngZone.run(function () {
                                                    _this.router.navigate(['/']);
                                                });
                                            }
                                            // console.log(run_data['error_code']);
                                        }
                                    });
                                }
                                else {
                                    if (wallet_index === data.length - 1 && openWallets_1 === 0) {
                                        _this.ngZone.run(function () {
                                            _this.router.navigate(['/']);
                                        });
                                    }
                                }
                            });
                        });
                    }
                    else {
                        _this.ngZone.run(function () {
                            _this.router.navigate(['/']);
                        });
                    }
                }
            });
        }
    };
    LoginComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-login',
            template: __webpack_require__(/*! ./login.component.html */ "./src/app/login/login.component.html"),
            styles: [__webpack_require__(/*! ./login.component.scss */ "./src/app/login/login.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_2__["ActivatedRoute"],
            _angular_router__WEBPACK_IMPORTED_MODULE_2__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], LoginComponent);
    return LoginComponent;
}());



/***/ }),

/***/ "./src/app/main/main.component.html":
/*!******************************************!*\
  !*** ./src/app/main/main.component.html ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\r\n  <div class=\"add-wallet\">\r\n    <h3 class=\"add-wallet-title\">{{ 'MAIN.TITLE' | translate }}</h3>\r\n    <div class=\"add-wallet-buttons\">\r\n      <button type=\"button\" class=\"blue-button\" [routerLink]=\"['/create']\">{{ 'MAIN.BUTTON_NEW_WALLET' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"openWallet()\">{{ 'MAIN.BUTTON_OPEN_WALLET' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button\" [routerLink]=\"['/restore']\">{{ 'MAIN.BUTTON_RESTORE_BACKUP' | translate }}</button>\r\n    </div>\r\n    <div class=\"add-wallet-help\">\r\n      <i class=\"icon\"></i><span>{{ 'MAIN.HELP' | translate }}</span>\r\n    </div>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/main/main.component.scss":
/*!******************************************!*\
  !*** ./src/app/main/main.component.scss ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  flex: 1 0 auto;\n  padding: 3rem; }\n\n.content {\n  padding: 3rem;\n  min-height: 100%; }\n\n.add-wallet .add-wallet-title {\n  margin-bottom: 1rem; }\n\n.add-wallet .add-wallet-buttons {\n  display: flex;\n  align-items: center;\n  justify-content: space-between;\n  margin: 0 -0.5rem;\n  padding: 1.5rem 0; }\n\n.add-wallet .add-wallet-buttons button {\n    flex: 1 0 auto;\n    margin: 0 0.5rem; }\n\n.add-wallet .add-wallet-help {\n  display: flex;\n  font-size: 1.3rem;\n  line-height: 1.4rem; }\n\n.add-wallet .add-wallet-help .icon {\n    -webkit-mask: url('howto.svg') no-repeat center;\n            mask: url('howto.svg') no-repeat center;\n    margin-right: 0.8rem;\n    width: 1.4rem;\n    height: 1.4rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvbWFpbi9EOlxcemFuby9zcmNcXGFwcFxcbWFpblxcbWFpbi5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGVBQWM7RUFDZCxjQUFhLEVBQ2Q7O0FBRUQ7RUFDRSxjQUFhO0VBQ2IsaUJBQWdCLEVBQ2pCOztBQUVEO0VBR0ksb0JBQW1CLEVBQ3BCOztBQUpIO0VBT0ksY0FBYTtFQUNiLG9CQUFtQjtFQUNuQiwrQkFBOEI7RUFDOUIsa0JBQWlCO0VBQ2pCLGtCQUFpQixFQU1sQjs7QUFqQkg7SUFjTSxlQUFjO0lBQ2QsaUJBQWdCLEVBQ2pCOztBQWhCTDtFQW9CSSxjQUFhO0VBQ2Isa0JBQWlCO0VBQ2pCLG9CQUFtQixFQVFwQjs7QUE5Qkg7SUF5Qk0sZ0RBQXdEO1lBQXhELHdDQUF3RDtJQUN4RCxxQkFBb0I7SUFDcEIsY0FBYTtJQUNiLGVBQWMsRUFDZiIsImZpbGUiOiJzcmMvYXBwL21haW4vbWFpbi5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBmbGV4OiAxIDAgYXV0bztcclxuICBwYWRkaW5nOiAzcmVtO1xyXG59XHJcblxyXG4uY29udGVudCB7XHJcbiAgcGFkZGluZzogM3JlbTtcclxuICBtaW4taGVpZ2h0OiAxMDAlO1xyXG59XHJcblxyXG4uYWRkLXdhbGxldCB7XHJcblxyXG4gIC5hZGQtd2FsbGV0LXRpdGxlIHtcclxuICAgIG1hcmdpbi1ib3R0b206IDFyZW07XHJcbiAgfVxyXG5cclxuICAuYWRkLXdhbGxldC1idXR0b25zIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAganVzdGlmeS1jb250ZW50OiBzcGFjZS1iZXR3ZWVuO1xyXG4gICAgbWFyZ2luOiAwIC0wLjVyZW07XHJcbiAgICBwYWRkaW5nOiAxLjVyZW0gMDtcclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBmbGV4OiAxIDAgYXV0bztcclxuICAgICAgbWFyZ2luOiAwIDAuNXJlbTtcclxuICAgIH1cclxuICB9XHJcblxyXG4gIC5hZGQtd2FsbGV0LWhlbHAge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbGluZS1oZWlnaHQ6IDEuNHJlbTtcclxuXHJcbiAgICAuaWNvbiB7XHJcbiAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvaG93dG8uc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICBtYXJnaW4tcmlnaHQ6IDAuOHJlbTtcclxuICAgICAgd2lkdGg6IDEuNHJlbTtcclxuICAgICAgaGVpZ2h0OiAxLjRyZW07XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcbiJdfQ== */"

/***/ }),

/***/ "./src/app/main/main.component.ts":
/*!****************************************!*\
  !*** ./src/app/main/main.component.ts ***!
  \****************************************/
/*! exports provided: MainComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "MainComponent", function() { return MainComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};




var MainComponent = /** @class */ (function () {
    function MainComponent(router, backend, variablesService, ngZone) {
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
    }
    MainComponent.prototype.ngOnInit = function () { };
    MainComponent.prototype.openWallet = function () {
        var _this = this;
        this.backend.openFileDialog('Open the wallet file.', '*', this.variablesService.settings.default_path, function (file_status, file_data) {
            if (file_status) {
                _this.variablesService.settings.default_path = file_data.path.substr(0, file_data.path.lastIndexOf('/'));
                _this.ngZone.run(function () {
                    _this.router.navigate(['/open'], { queryParams: { path: file_data.path } });
                });
            }
            else {
                console.log(file_data['error_code']);
            }
        });
    };
    MainComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-main',
            template: __webpack_require__(/*! ./main.component.html */ "./src/app/main/main.component.html"),
            styles: [__webpack_require__(/*! ./main.component.scss */ "./src/app/main/main.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_3__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_1__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], MainComponent);
    return MainComponent;
}());



/***/ }),

/***/ "./src/app/messages/messages.component.html":
/*!**************************************************!*\
  !*** ./src/app/messages/messages.component.html ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"wrap-table\">\r\n  <table class=\"messages-table\">\r\n    <thead>\r\n    <tr>\r\n      <th>{{ 'MESSAGES.ADDRESS' | translate }}</th>\r\n      <th>{{ 'MESSAGES.MESSAGE' | translate }}</th>\r\n    </tr>\r\n    </thead>\r\n    <tbody>\r\n    <tr *ngFor=\"let message of messages\" [routerLink]=\"[message.address]\">\r\n      <td>\r\n        <span>{{message.address}}</span>\r\n        <i class=\"icon\" *ngIf=\"message.is_new\"></i>\r\n      </td>\r\n      <td>\r\n        <span>{{message.message}}</span>\r\n      </td>\r\n    </tr>\r\n    </tbody>\r\n  </table>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/messages/messages.component.scss":
/*!**************************************************!*\
  !*** ./src/app/messages/messages.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.wrap-table {\n  margin: -3rem; }\n\n.wrap-table table tbody tr td:first-child {\n    position: relative;\n    padding-right: 5rem;\n    width: 18rem; }\n\n.wrap-table table tbody tr td:first-child span {\n      display: block;\n      line-height: 3.5rem;\n      max-width: 10rem; }\n\n.wrap-table table tbody tr td:first-child .icon {\n      position: absolute;\n      top: 50%;\n      right: 1rem;\n      -webkit-transform: translateY(-50%);\n              transform: translateY(-50%);\n      display: block;\n      -webkit-mask: url('alert.svg') no-repeat 0;\n              mask: url('alert.svg') no-repeat 0;\n      width: 1.2rem;\n      height: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvbWVzc2FnZXMvRDpcXHphbm8vc3JjXFxhcHBcXG1lc3NhZ2VzXFxtZXNzYWdlcy5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLFlBQVcsRUFDWjs7QUFFRDtFQUNFLGNBQWEsRUFvQ2Q7O0FBckNEO0lBWVksbUJBQWtCO0lBQ2xCLG9CQUFtQjtJQUNuQixhQUFZLEVBa0JiOztBQWhDWDtNQWlCYyxlQUFjO01BQ2Qsb0JBQW1CO01BQ25CLGlCQUFnQixFQUNqQjs7QUFwQmI7TUF1QmMsbUJBQWtCO01BQ2xCLFNBQVE7TUFDUixZQUFXO01BQ1gsb0NBQTJCO2NBQTNCLDRCQUEyQjtNQUMzQixlQUFjO01BQ2QsMkNBQW1EO2NBQW5ELG1DQUFtRDtNQUNuRCxjQUFhO01BQ2IsZUFBYyxFQUNmIiwiZmlsZSI6InNyYy9hcHAvbWVzc2FnZXMvbWVzc2FnZXMuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XHJcbiAgd2lkdGg6IDEwMCU7XHJcbn1cclxuXHJcbi53cmFwLXRhYmxlIHtcclxuICBtYXJnaW46IC0zcmVtO1xyXG5cclxuICB0YWJsZSB7XHJcblxyXG4gICAgdGJvZHkge1xyXG5cclxuICAgICAgdHIge1xyXG5cclxuICAgICAgICB0ZCB7XHJcblxyXG4gICAgICAgICAgJjpmaXJzdC1jaGlsZCB7XHJcbiAgICAgICAgICAgIHBvc2l0aW9uOiByZWxhdGl2ZTtcclxuICAgICAgICAgICAgcGFkZGluZy1yaWdodDogNXJlbTtcclxuICAgICAgICAgICAgd2lkdGg6IDE4cmVtO1xyXG5cclxuICAgICAgICAgICAgc3BhbiB7XHJcbiAgICAgICAgICAgICAgZGlzcGxheTogYmxvY2s7XHJcbiAgICAgICAgICAgICAgbGluZS1oZWlnaHQ6IDMuNXJlbTtcclxuICAgICAgICAgICAgICBtYXgtd2lkdGg6IDEwcmVtO1xyXG4gICAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgICAuaWNvbiB7XHJcbiAgICAgICAgICAgICAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gICAgICAgICAgICAgIHRvcDogNTAlO1xyXG4gICAgICAgICAgICAgIHJpZ2h0OiAxcmVtO1xyXG4gICAgICAgICAgICAgIHRyYW5zZm9ybTogdHJhbnNsYXRlWSgtNTAlKTtcclxuICAgICAgICAgICAgICBkaXNwbGF5OiBibG9jaztcclxuICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2FsZXJ0LnN2Zykgbm8tcmVwZWF0IDA7XHJcbiAgICAgICAgICAgICAgd2lkdGg6IDEuMnJlbTtcclxuICAgICAgICAgICAgICBoZWlnaHQ6IDEuMnJlbTtcclxuICAgICAgICAgICAgfVxyXG4gICAgICAgICAgfVxyXG4gICAgICAgIH1cclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

/***/ }),

/***/ "./src/app/messages/messages.component.ts":
/*!************************************************!*\
  !*** ./src/app/messages/messages.component.ts ***!
  \************************************************/
/*! exports provided: MessagesComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "MessagesComponent", function() { return MessagesComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};

var MessagesComponent = /** @class */ (function () {
    function MessagesComponent() {
        this.messages = [
            {
                is_new: true,
                address: '@bitmap',
                message: 'No more miners for you!'
            },
            {
                is_new: false,
                address: 'Hjkwey36gHasdhkajshd4bxnb5mcvowyefb2633FdsFGGWbb',
                message: 'Hey! What’s with our BBR deal?'
            },
            {
                is_new: false,
                address: '@john',
                message: 'I’m coming!'
            }
        ];
    }
    MessagesComponent.prototype.ngOnInit = function () { };
    MessagesComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-messages',
            template: __webpack_require__(/*! ./messages.component.html */ "./src/app/messages/messages.component.html"),
            styles: [__webpack_require__(/*! ./messages.component.scss */ "./src/app/messages/messages.component.scss")]
        }),
        __metadata("design:paramtypes", [])
    ], MessagesComponent);
    return MessagesComponent;
}());



/***/ }),

/***/ "./src/app/open-wallet/open-wallet.component.html":
/*!********************************************************!*\
  !*** ./src/app/open-wallet/open-wallet.component.html ***!
  \********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span>{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\r\n      <span>{{ 'BREADCRUMBS.OPEN_WALLET' | translate }}</span>\r\n    </div>\r\n    <a class=\"back-btn\" [routerLink]=\"['/main']\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </a>\r\n  </div>\r\n\r\n  <form class=\"form-open\" [formGroup]=\"openForm\">\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-name\">{{ 'OPEN_WALLET.NAME' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"openForm.controls['name'].invalid && (openForm.controls['name'].dirty || openForm.controls['name'].touched)\">\r\n      <div *ngIf=\"openForm.controls['name'].errors['required']\">\r\n        Name is required.\r\n      </div>\r\n      <div *ngIf=\"openForm.controls['name'].errors['duplicate']\">\r\n        Name is duplicate.\r\n      </div>\r\n    </div>\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-password\">{{ 'OPEN_WALLET.PASS' | translate }}</label>\r\n      <input type=\"password\" id=\"wallet-password\" formControlName=\"password\">\r\n    </div>\r\n    <div class=\"wrap-buttons\">\r\n      <button type=\"button\" class=\"blue-button create-button\" (click)=\"openWallet()\" [disabled]=\"!openForm.valid\">{{ 'OPEN_WALLET.BUTTON' | translate }}</button>\r\n    </div>\r\n  </form>\r\n\r\n</div>\r\n\r\n"

/***/ }),

/***/ "./src/app/open-wallet/open-wallet.component.scss":
/*!********************************************************!*\
  !*** ./src/app/open-wallet/open-wallet.component.scss ***!
  \********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-open {\n  margin: 2.4rem 0;\n  width: 50%; }\n  .form-open .wrap-buttons {\n    display: flex;\n    margin: 2.5rem -0.7rem; }\n  .form-open .wrap-buttons button {\n      margin: 0 0.7rem; }\n  .form-open .wrap-buttons button.create-button {\n        flex: 1 1 50%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvb3Blbi13YWxsZXQvRDpcXHphbm8vc3JjXFxhcHBcXG9wZW4td2FsbGV0XFxvcGVuLXdhbGxldC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGlCQUFnQjtFQUNoQixXQUFVLEVBY1g7RUFoQkQ7SUFLSSxjQUFhO0lBQ2IsdUJBQXNCLEVBU3ZCO0VBZkg7TUFTTSxpQkFBZ0IsRUFLakI7RUFkTDtRQVlRLGNBQWEsRUFDZCIsImZpbGUiOiJzcmMvYXBwL29wZW4td2FsbGV0L29wZW4td2FsbGV0LmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tb3BlbiB7XHJcbiAgbWFyZ2luOiAyLjRyZW0gMDtcclxuICB3aWR0aDogNTAlO1xyXG5cclxuICAud3JhcC1idXR0b25zIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBtYXJnaW46IDIuNXJlbSAtMC43cmVtO1xyXG5cclxuICAgIGJ1dHRvbiB7XHJcbiAgICAgIG1hcmdpbjogMCAwLjdyZW07XHJcblxyXG4gICAgICAmLmNyZWF0ZS1idXR0b24ge1xyXG4gICAgICAgIGZsZXg6IDEgMSA1MCU7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcbn1cclxuIl19 */"

/***/ }),

/***/ "./src/app/open-wallet/open-wallet.component.ts":
/*!******************************************************!*\
  !*** ./src/app/open-wallet/open-wallet.component.ts ***!
  \******************************************************/
/*! exports provided: OpenWalletComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "OpenWalletComponent", function() { return OpenWalletComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/models/wallet.model */ "./src/app/_helpers/models/wallet.model.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var OpenWalletComponent = /** @class */ (function () {
    function OpenWalletComponent(route, router, backend, variablesService, ngZone) {
        var _this = this;
        this.route = route;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.openForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            name: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [_angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required, function (g) {
                    for (var i = 0; i < _this.variablesService.wallets.length; i++) {
                        if (g.value === _this.variablesService.wallets[i].name) {
                            return { 'duplicate': true };
                        }
                    }
                    return null;
                }]),
            password: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('')
        });
    }
    OpenWalletComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.route.queryParams.subscribe(function (params) {
            if (params.path) {
                _this.filePath = params.path;
                var filename = '';
                if (params.path.lastIndexOf('.') === -1) {
                    filename = params.path.substr(params.path.lastIndexOf('/') + 1);
                }
                else {
                    filename = params.path.substr(params.path.lastIndexOf('/') + 1, params.path.lastIndexOf('.') - 1 - params.path.lastIndexOf('/'));
                }
                if (filename.length > 25) {
                    filename = filename.slice(0, 25);
                }
                _this.openForm.get('name').setValue(filename);
            }
        });
    };
    OpenWalletComponent.prototype.openWallet = function () {
        var _this = this;
        if (this.openForm.valid) {
            this.backend.openWallet(this.filePath, this.openForm.get('password').value, false, function (open_status, open_data, open_error) {
                if (open_error && open_error === 'FILE_NOT_FOUND') {
                    // var error_translate = $filter('translate')('INFORMER.SAFE_FILE_NOT_FOUND1');
                    // error_translate += ':<br>' + $scope.safe.path;
                    // error_translate += $filter('translate')('INFORMER.SAFE_FILE_NOT_FOUND2');
                    // informer.fileNotFound(error_translate);
                    alert('FILE_NOT_FOUND');
                }
                else {
                    if (open_status || open_error === 'FILE_RESTORED') {
                        var exists_1 = false;
                        _this.variablesService.wallets.forEach(function (wallet) {
                            if (wallet.address === open_data['wi'].address) {
                                exists_1 = true;
                            }
                        });
                        if (exists_1) {
                            alert('SAFES.WITH_ADDRESS_ALREADY_OPEN');
                            _this.backend.closeWallet(open_data.wallet_id, function (close_status, close_data) {
                                console.log(close_status, close_data);
                                _this.ngZone.run(function () {
                                    _this.router.navigate(['/']);
                                });
                            });
                        }
                        else {
                            _this.backend.runWallet(open_data.wallet_id, function (run_status, run_data) {
                                if (run_status) {
                                    var new_wallet_1 = new _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_5__["Wallet"](open_data.wallet_id, _this.openForm.get('name').value, _this.openForm.get('password').value, open_data['wi'].path, open_data['wi'].address, open_data['wi'].balance, open_data['wi'].unlocked_balance, open_data['wi'].mined_total, open_data['wi'].tracking_hey);
                                    if (open_data.recent_history && open_data.recent_history.history) {
                                        new_wallet_1.prepareHistory(open_data.recent_history.history);
                                    }
                                    _this.backend.getContracts(open_data.wallet_id, function (contracts_status, contracts_data) {
                                        if (contracts_status && contracts_data.hasOwnProperty('contracts')) {
                                            _this.ngZone.run(function () {
                                                new_wallet_1.prepareContractsAfterOpen(contracts_data.contracts, _this.variablesService.exp_med_ts, _this.variablesService.height_app, _this.variablesService.settings.viewedContracts, _this.variablesService.settings.notViewedContracts);
                                            });
                                        }
                                    });
                                    _this.variablesService.wallets.push(new_wallet_1);
                                    _this.backend.storeSecureAppData(function (status, data) {
                                        console.log('Store App Data', status, data);
                                    });
                                    _this.ngZone.run(function () {
                                        _this.router.navigate(['/wallet/' + open_data.wallet_id]);
                                    });
                                }
                                else {
                                    console.log(run_data['error_code']);
                                }
                            });
                        }
                    }
                }
            });
        }
    };
    OpenWalletComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-open-wallet',
            template: __webpack_require__(/*! ./open-wallet.component.html */ "./src/app/open-wallet/open-wallet.component.html"),
            styles: [__webpack_require__(/*! ./open-wallet.component.scss */ "./src/app/open-wallet/open-wallet.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_4__["ActivatedRoute"],
            _angular_router__WEBPACK_IMPORTED_MODULE_4__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], OpenWalletComponent);
    return OpenWalletComponent;
}());



/***/ }),

/***/ "./src/app/purchase/purchase.component.html":
/*!**************************************************!*\
  !*** ./src/app/purchase/purchase.component.html ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"head\">\r\n  <div class=\"breadcrumbs\">\r\n    <span>{{ 'BREADCRUMBS.CONTRACTS' | translate }}</span>\r\n    <span *ngIf=\"newPurchase\">{{ 'BREADCRUMBS.NEW_PURCHASE' | translate }}</span>\r\n    <span *ngIf=\"!newPurchase\">{{ 'BREADCRUMBS.OLD_PURCHASE' | translate }}</span>\r\n  </div>\r\n  <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n    <i class=\"icon back\"></i>\r\n    <span>{{ 'COMMON.BACK' | translate }}</span>\r\n  </button>\r\n</div>\r\n\r\n<form class=\"form-purchase scrolled-content\" [formGroup]=\"purchaseForm\">\r\n\r\n  <div class=\"input-block\">\r\n    <label for=\"purchase-description\">{{ 'PURCHASE.DESCRIPTION' | translate }}</label>\r\n    <input type=\"text\" id=\"purchase-description\" formControlName=\"description\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n  </div>\r\n  <div class=\"error-block\" *ngIf=\"purchaseForm.controls['description'].invalid && (purchaseForm.controls['description'].dirty || purchaseForm.controls['description'].touched)\">\r\n    <div *ngIf=\"purchaseForm.controls['description'].errors['required']\">\r\n      Description is required.\r\n    </div>\r\n  </div>\r\n\r\n  <div class=\"input-blocks-row\">\r\n    <div class=\"input-block\">\r\n      <label for=\"purchase-seller\">{{ 'PURCHASE.SELLER' | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-seller\" formControlName=\"seller\" (blur)=\"checkAddressValidation()\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"purchaseForm.controls['seller'].invalid && (purchaseForm.controls['seller'].dirty || purchaseForm.controls['seller'].touched)\">\r\n      <div *ngIf=\"purchaseForm.controls['seller'].errors['required']\">\r\n        Seller is required.\r\n      </div>\r\n      <div *ngIf=\"purchaseForm.controls['seller'].errors['address_not_valid']\">\r\n        Seller not valid.\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"purchase-amount\">{{ 'PURCHASE.AMOUNT' | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-amount\" formControlName=\"amount\" appInputValidate=\"money\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"purchaseForm.controls['amount'].invalid && (purchaseForm.controls['amount'].dirty || purchaseForm.controls['amount'].touched)\">\r\n      <div *ngIf=\"purchaseForm.controls['amount'].errors['required']\">\r\n        Amount is required.\r\n      </div>\r\n    </div>\r\n  </div>\r\n\r\n  <div class=\"input-blocks-row\">\r\n    <div class=\"input-block\">\r\n      <label for=\"purchase-your-deposit\">{{ 'PURCHASE.YOUR_DEPOSIT' | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-your-deposit\" formControlName=\"yourDeposit\" appInputValidate=\"money\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"purchaseForm.controls['yourDeposit'].invalid && (purchaseForm.controls['yourDeposit'].dirty || purchaseForm.controls['yourDeposit'].touched)\">\r\n      <div *ngIf=\"purchaseForm.controls['yourDeposit'].errors['required']\">\r\n        Your deposit is required.\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <div class=\"wrap-label\">\r\n        <label for=\"purchase-seller-deposit\">{{ 'PURCHASE.SELLER_DEPOSIT' | translate }}</label>\r\n        <div class=\"checkbox-block\">\r\n          <input type=\"checkbox\" id=\"purchase-same-amount\" class=\"style-checkbox\" formControlName=\"sameAmount\" (change)=\"sameAmountChange()\">\r\n          <label for=\"purchase-same-amount\">Same amount</label>\r\n        </div>\r\n      </div>\r\n      <input type=\"text\" readonly *ngIf=\"purchaseForm.controls['sameAmount'].value\" [value]=\"purchaseForm.controls['amount'].value\">\r\n      <input type=\"text\" id=\"purchase-seller-deposit\" *ngIf=\"!purchaseForm.controls['sameAmount'].value\" formControlName=\"sellerDeposit\" appInputValidate=\"money\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"purchaseForm.controls['sellerDeposit'].invalid && (purchaseForm.controls['sellerDeposit'].dirty || purchaseForm.controls['sellerDeposit'].touched)\">\r\n      <div *ngIf=\"purchaseForm.controls['sellerDeposit'].errors['required']\">\r\n        Seller deposit is required.\r\n      </div>\r\n    </div>\r\n  </div>\r\n\r\n  <div class=\"input-block\">\r\n    <label for=\"purchase-comment\">{{ 'PURCHASE.COMMENT' | translate }}</label>\r\n    <input type=\"text\" id=\"purchase-comment\" formControlName=\"comment\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n  </div>\r\n\r\n  <button type=\"button\" class=\"purchase-select\" (click)=\"toggleOptions()\">\r\n    <span>{{ 'PURCHASE.DETAILS' | translate }}</span><i class=\"icon arrow\" [class.down]=\"!additionalOptions\" [class.up]=\"additionalOptions\"></i>\r\n  </button>\r\n\r\n  <div class=\"additional-details\" *ngIf=\"additionalOptions\">\r\n    <div class=\"input-block\">\r\n      <label for=\"purchase-fee\">{{ 'PURCHASE.FEE' | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-fee\" formControlName=\"fee\" readonly>\r\n    </div>\r\n    <div class=\"input-block\">\r\n      <label for=\"purchase-payment\">{{ 'PURCHASE.PAYMENT' | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-payment\" formControlName=\"payment\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n  </div>\r\n\r\n  <button type=\"button\" class=\"blue-button send-button\" *ngIf=\"newPurchase\" [disabled]=\"!purchaseForm.valid\" (click)=\"createPurchase()\">{{ 'PURCHASE.SEND_BUTTON' | translate }}</button>\r\n\r\n  <div class=\"purchase-states\" *ngIf=\"!newPurchase\">\r\n\r\n    <ng-container *ngIf=\"currentContract.state == 1 && !currentContract.is_a && (currentContract.private_detailes.b_pledge + ('0.01' | moneyToInt) + ('0.01' | moneyToInt)) > variablesService.currentWallet.unlocked_balance\">\r\n      <span>{{ 'There are insufficient funds in the safe. Add funds to the safe to continue' | translate }}</span>\r\n    </ng-container>\r\n\r\n    <ng-container *ngIf=\"currentContract.is_a\">\r\n      <span *ngIf=\"currentContract.state == 1\">{{ 'DEALS.CUSTOMER_WAITING_ANSWER' | translate }}</span>\r\n      <!--<span *ngIf=\"currentContract.state == 1\" ng-bind=\"'(' + (currentContract.expiration_time | buyingTime : 0) + ')'\"></span>-->\r\n\r\n      <span *ngIf=\"currentContract.state == 110\">{{ 'The seller ignored your contract proposal' | translate }}</span>\r\n      <span *ngIf=\"currentContract.state == 110\">{{ 'Pledge amount unblocked' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 120\">{{ 'Waiting for seller to ship item' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 130\">{{ 'The seller ignored your proposal to cancel the contract' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 140\">{{ 'The contract proposal has expired' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 201\">{{ 'Please wait for the pledges to be made' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 2\">{{ 'Waiting for seller to ship item' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 3\">{{ 'Contract completed successfully' | translate }}</span>\r\n      <span *ngIf=\"currentContract.state == 3\">{{ 'Item received, payment transferred, pledges returned' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 4\">{{ 'Item not received' | translate }}</span>\r\n      <span *ngIf=\"currentContract.state == 4\">{{ 'All pledges nullified' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 5\">{{ 'You have made a proposal to cancel the contract' | translate }}</span>\r\n      <!--<span *ngIf=\"currentContract.state == 5\" ng-bind=\"'(' + (contract.cancel_expiration_time | buyingTime : 2) + ')'\"></span>-->\r\n\r\n      <span *ngIf=\"currentContract.state == 601\">{{ 'The contract is being cancelled. Please wait for the pledge to be returned' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 6\">{{ 'Contract canceled' | translate }}</span>\r\n      <span *ngIf=\"currentContract.state == 6\">{{ 'Pledges returned' | translate }}</span>\r\n    </ng-container>\r\n\r\n    <ng-container *ngIf=\"!currentContract.is_a\">\r\n      <span *ngIf=\"currentContract.state == 1\">{{ 'The buyer is proposing a contract' | translate }}</span>\r\n      <!--<span *ngIf=\"currentContract.state == 1\" ng-bind=\"'(' + (contract.expiration_time | buyingTime : 1) + ')'\"></span>-->\r\n\r\n      <span *ngIf=\"currentContract.state == 110\">{{ 'You ignored the contract proposal' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 130\">{{ 'You ignored the proposal to cancel the contract' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 140\">{{ 'The contract proposal has expired' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 201\">{{ 'Please wait for the pledges to be made' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 2\">{{ 'The buyer is waiting for the item to be delivered' | translate }}</span>\r\n      <span *ngIf=\"currentContract.state == 2\">{{ 'Pledges made' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 3\">{{ 'Contract completed successfully' | translate }}</span>\r\n      <span *ngIf=\"currentContract.state == 3\">{{ 'Item received, payment transferred, pledges returned' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 4\">{{ 'Item not received' | translate }}</span>\r\n      <span *ngIf=\"currentContract.state == 4\">{{ 'All pledges nullified' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 5\">{{ 'The buyer is offering to cancel the contract and return the pledge' | translate }}</span>\r\n      <!--<span *ngIf=\"currentContract.state == 5\" ng-bind=\"'(' + (contract.cancel_expiration_time | buyingTime : 1) + ')'\"></span>-->\r\n\r\n      <span *ngIf=\"currentContract.state == 601\">{{ 'The contract is being cancelled. Please wait for the pledge to be returned' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 6\">{{ 'Contract canceled' | translate }}</span>\r\n      <span *ngIf=\"currentContract.state == 6\">{{ 'Pledges returned' | translate }}</span>\r\n    </ng-container>\r\n\r\n    <ng-container *ngIf=\"currentContract.state == 201 || currentContract.state == 601\">\r\n      <span *ngIf=\"(variablesService.height_app - currentContract.height) < 10\">{{variablesService.height_app - currentContract.height}}/10</span>\r\n      <span *ngIf=\"historyBlock && historyBlock.sortAmount\">{{(historyBlock.is_income ? '+' : '') + (historyBlock.sortAmount | intToMoney)}}</span>\r\n    </ng-container>\r\n\r\n  </div>\r\n\r\n  <div class=\"purchase-buttons\" *ngIf=\"!newPurchase\">\r\n    <!--<button type=\"button\" class=\"blue-button\">{{ 'PURCHASE.CANCEL_BUTTON' | translate }}</button>-->\r\n    <!--<button type=\"button\" class=\"turquoise-button\">{{ 'PURCHASE.TERMINATE_BUTTON' | translate }}</button>-->\r\n    <!--<button type=\"button\" class=\"green-button\">{{ 'PURCHASE.COMPLETE_BUTTON' | translate }}</button>-->\r\n\r\n    <ng-container *ngIf=\"!currentContract.is_a && currentContract.state == 1\">\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"acceptState();\" [disabled]=\"(currentContract.private_detailes.b_pledge + ('0.01' | moneyToInt) + ('0.01' | moneyToInt)) > variablesService.currentWallet.unlocked_balance\">\r\n        {{'Accept (Make pledge)' | translate}}\r\n      </button>\r\n      <button type=\"button\" class=\"turquoise-button\" (click)=\"ignoredContract();\">{{'Ignore' | translate}}</button>\r\n    </ng-container>\r\n\r\n    <ng-container *ngIf=\"currentContract.is_a && (currentContract.state == 201 || currentContract.state == 2 || currentContract.state == 120 || currentContract.state == 130)\">\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"productNotGot();\" [disabled]=\"currentContract.cancel_expiration_time == 0 && (currentContract.height == 0 || (variablesService.height_app - currentContract.height) < 10)\">\r\n        {{'Item not received (Nullify pledges)' | translate}}\r\n      </button>\r\n      <button type=\"button\" class=\"turquoise-button\" (click)=\"dealsDetailsFinish();\" [disabled]=\"currentContract.cancel_expiration_time == 0 && (currentContract.height == 0 || (variablesService.height_app - currentContract.height) < 10)\">\r\n        {{'Item received (Transfer payment and return pledge to seller)' | translate}}\r\n      </button>\r\n      <button type=\"button\" class=\"green-button\" (click)=\"dealsDetailsCancel();\" [disabled]=\"currentContract.cancel_expiration_time == 0 && (currentContract.height == 0 || (variablesService.height_app - currentContract.height) < 10)\">\r\n        {{'Cancel contract (Return pledge)' | translate}}\r\n      </button>\r\n    </ng-container>\r\n\r\n    <ng-container *ngIf=\"!currentContract.is_a && currentContract.state == 5\">\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"dealsDetailsDontCanceling();\">{{'Do not cancel (Item shipped)' | translate}}</button>\r\n      <button type=\"button\" class=\"turquoise-button\" (click)=\"dealsDetailsSellerCancel();\">{{'Cancel contract (Return pledge)' | translate}}</button>\r\n    </ng-container>\r\n\r\n  </div>\r\n\r\n</form>\r\n\r\n<div class=\"progress-bar-container\">\r\n  <div class=\"progress-bar\">\r\n    <div class=\"progress-bar-full\" [style.width]=\"getProgressBarWidth()\"></div>\r\n  </div>\r\n  <div class=\"progress-labels\">\r\n    <span>{{ 'PURCHASE.PROGRESS_NEW' | translate }}</span>\r\n    <span>{{ 'PURCHASE.PROGRESS_WAIT' | translate }}</span>\r\n    <span>{{ 'PURCHASE.PROGRESS_COMPLETE' | translate }}</span>\r\n  </div>\r\n  <div class=\"progress-time\" *ngIf=\"!newPurchase\">\r\n    <span *ngIf=\"currentContract.is_a && currentContract.state == 1\">{{currentContract.expiration_time | contractTimeLeft: 0}}</span>\r\n    <span *ngIf=\"currentContract.is_a && currentContract.state == 5\">{{currentContract.cancel_expiration_time | contractTimeLeft: 2}}</span>\r\n    <span *ngIf=\"!currentContract.is_a && currentContract.state == 1\">{{currentContract.expiration_time | contractTimeLeft: 1}}</span>\r\n    <span *ngIf=\"!currentContract.is_a && currentContract.state == 5\">{{currentContract.cancel_expiration_time | contractTimeLeft: 1}}</span>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/purchase/purchase.component.scss":
/*!**************************************************!*\
  !*** ./src/app/purchase/purchase.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: flex;\n  flex-direction: column;\n  width: 100%; }\n\n.head {\n  flex: 0 0 auto;\n  box-sizing: content-box;\n  margin: -3rem -3rem 0; }\n\n.form-purchase {\n  flex: 1 1 auto;\n  margin: 1.5rem -3rem 0;\n  padding: 0 3rem;\n  overflow-y: overlay; }\n\n.form-purchase .input-blocks-row {\n    display: flex; }\n\n.form-purchase .input-blocks-row .input-block {\n      flex-basis: 50%; }\n\n.form-purchase .input-blocks-row .input-block:first-child {\n        margin-right: 1.5rem; }\n\n.form-purchase .input-blocks-row .input-block:last-child {\n        margin-left: 1.5rem; }\n\n.form-purchase .input-blocks-row .input-block .checkbox-block {\n        display: flex; }\n\n.form-purchase .purchase-select {\n    display: flex;\n    align-items: center;\n    background: transparent;\n    border: none;\n    font-size: 1.3rem;\n    line-height: 1.3rem;\n    margin: 1.5rem 0 0;\n    padding: 0;\n    width: 100%;\n    max-width: 15rem;\n    height: 1.3rem; }\n\n.form-purchase .purchase-select .arrow {\n      margin-left: 1rem;\n      width: 0.8rem;\n      height: 0.8rem; }\n\n.form-purchase .purchase-select .arrow.down {\n        -webkit-mask: url('arrow-down.svg') no-repeat center;\n                mask: url('arrow-down.svg') no-repeat center; }\n\n.form-purchase .purchase-select .arrow.up {\n        -webkit-mask: url('arrow-up.svg') no-repeat center;\n                mask: url('arrow-up.svg') no-repeat center; }\n\n.form-purchase .additional-details {\n    display: flex;\n    margin-top: 1.5rem;\n    padding: 0.5rem 0 2rem; }\n\n.form-purchase .additional-details > div {\n      flex-basis: 25%; }\n\n.form-purchase .additional-details > div:first-child {\n        padding-left: 1.5rem;\n        padding-right: 1rem; }\n\n.form-purchase .additional-details > div:last-child {\n        padding-left: 1rem;\n        padding-right: 1.5rem; }\n\n.form-purchase .purchase-states {\n    display: flex;\n    flex-direction: column;\n    align-items: center;\n    justify-content: center;\n    font-size: 1.2rem;\n    line-height: 2.9rem; }\n\n.form-purchase .send-button {\n    margin: 2.4rem 0;\n    width: 100%;\n    max-width: 15rem; }\n\n.form-purchase .purchase-buttons {\n    display: flex;\n    justify-content: space-between;\n    margin: 2.4rem -0.5rem;\n    width: calc(100% + 1rem); }\n\n.form-purchase .purchase-buttons button {\n      flex: 0 1 33%;\n      margin: 0 0.5rem; }\n\n.progress-bar-container {\n  position: absolute;\n  bottom: 0;\n  left: 0;\n  padding: 0 3rem;\n  width: 100%;\n  height: 3rem; }\n\n.progress-bar-container .progress-bar {\n    position: absolute;\n    top: -0.7rem;\n    left: 0;\n    margin: 0 3rem;\n    width: calc(100% - 6rem);\n    height: 0.7rem; }\n\n.progress-bar-container .progress-bar .progress-bar-full {\n      height: 0.7rem; }\n\n.progress-bar-container .progress-labels {\n    display: flex;\n    align-items: center;\n    justify-content: space-between;\n    font-size: 1.2rem;\n    height: 100%; }\n\n.progress-bar-container .progress-labels span {\n      flex: 1 0 0;\n      text-align: center; }\n\n.progress-bar-container .progress-labels span:first-child {\n        text-align: left; }\n\n.progress-bar-container .progress-labels span:last-child {\n        text-align: right; }\n\n.progress-bar-container .progress-time {\n    position: absolute;\n    top: -3rem;\n    left: 50%;\n    -webkit-transform: translateX(-50%);\n            transform: translateX(-50%);\n    font-size: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvcHVyY2hhc2UvRDpcXHphbm8vc3JjXFxhcHBcXHB1cmNoYXNlXFxwdXJjaGFzZS5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGNBQWE7RUFDYix1QkFBc0I7RUFDdEIsWUFBVyxFQUNaOztBQUVEO0VBQ0UsZUFBYztFQUNkLHdCQUF1QjtFQUN2QixzQkFBcUIsRUFDdEI7O0FBRUQ7RUFDRSxlQUFjO0VBQ2QsdUJBQXNCO0VBQ3RCLGdCQUFlO0VBQ2Ysb0JBQW1CLEVBZ0dwQjs7QUFwR0Q7SUFPSSxjQUFhLEVBaUJkOztBQXhCSDtNQVVNLGdCQUFlLEVBYWhCOztBQXZCTDtRQWFRLHFCQUFvQixFQUNyQjs7QUFkUDtRQWlCUSxvQkFBbUIsRUFDcEI7O0FBbEJQO1FBcUJRLGNBQWEsRUFDZDs7QUF0QlA7SUEyQkksY0FBYTtJQUNiLG9CQUFtQjtJQUNuQix3QkFBdUI7SUFDdkIsYUFBWTtJQUNaLGtCQUFpQjtJQUNqQixvQkFBbUI7SUFDbkIsbUJBQWtCO0lBQ2xCLFdBQVU7SUFDVixZQUFXO0lBQ1gsaUJBQWdCO0lBQ2hCLGVBQWMsRUFlZjs7QUFwREg7TUF3Q00sa0JBQWlCO01BQ2pCLGNBQWE7TUFDYixlQUFjLEVBU2Y7O0FBbkRMO1FBNkNRLHFEQUE0RDtnQkFBNUQsNkNBQTRELEVBQzdEOztBQTlDUDtRQWlEUSxtREFBMEQ7Z0JBQTFELDJDQUEwRCxFQUMzRDs7QUFsRFA7SUF1REksY0FBYTtJQUNiLG1CQUFrQjtJQUNsQix1QkFBc0IsRUFldkI7O0FBeEVIO01BNERNLGdCQUFlLEVBV2hCOztBQXZFTDtRQStEUSxxQkFBb0I7UUFDcEIsb0JBQW1CLEVBQ3BCOztBQWpFUDtRQW9FUSxtQkFBa0I7UUFDbEIsc0JBQXFCLEVBQ3RCOztBQXRFUDtJQTJFSSxjQUFhO0lBQ2IsdUJBQXNCO0lBQ3RCLG9CQUFtQjtJQUNuQix3QkFBdUI7SUFDdkIsa0JBQWlCO0lBQ2pCLG9CQUFtQixFQUNwQjs7QUFqRkg7SUFvRkksaUJBQWdCO0lBQ2hCLFlBQVc7SUFDWCxpQkFBZ0IsRUFDakI7O0FBdkZIO0lBMEZJLGNBQWE7SUFDYiwrQkFBOEI7SUFDOUIsdUJBQXNCO0lBQ3RCLHlCQUF3QixFQU16Qjs7QUFuR0g7TUFnR00sY0FBYTtNQUNiLGlCQUFnQixFQUNqQjs7QUFJTDtFQUNFLG1CQUFrQjtFQUNsQixVQUFTO0VBQ1QsUUFBTztFQUNQLGdCQUFlO0VBQ2YsWUFBVztFQUNYLGFBQVksRUEyQ2I7O0FBakREO0lBU0ksbUJBQWtCO0lBQ2xCLGFBQVk7SUFDWixRQUFPO0lBQ1AsZUFBYztJQUNkLHlCQUF3QjtJQUN4QixlQUFjLEVBS2Y7O0FBbkJIO01BaUJNLGVBQWMsRUFDZjs7QUFsQkw7SUFzQkksY0FBYTtJQUNiLG9CQUFtQjtJQUNuQiwrQkFBOEI7SUFDOUIsa0JBQWlCO0lBQ2pCLGFBQVksRUFjYjs7QUF4Q0g7TUE2Qk0sWUFBVztNQUNYLG1CQUFrQixFQVNuQjs7QUF2Q0w7UUFpQ1EsaUJBQWdCLEVBQ2pCOztBQWxDUDtRQXFDUSxrQkFBaUIsRUFDbEI7O0FBdENQO0lBMkNJLG1CQUFrQjtJQUNsQixXQUFVO0lBQ1YsVUFBUztJQUNULG9DQUEyQjtZQUEzQiw0QkFBMkI7SUFDM0Isa0JBQWlCLEVBQ2xCIiwiZmlsZSI6InNyYy9hcHAvcHVyY2hhc2UvcHVyY2hhc2UuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gIHdpZHRoOiAxMDAlO1xyXG59XHJcblxyXG4uaGVhZCB7XHJcbiAgZmxleDogMCAwIGF1dG87XHJcbiAgYm94LXNpemluZzogY29udGVudC1ib3g7XHJcbiAgbWFyZ2luOiAtM3JlbSAtM3JlbSAwO1xyXG59XHJcblxyXG4uZm9ybS1wdXJjaGFzZSB7XHJcbiAgZmxleDogMSAxIGF1dG87XHJcbiAgbWFyZ2luOiAxLjVyZW0gLTNyZW0gMDtcclxuICBwYWRkaW5nOiAwIDNyZW07XHJcbiAgb3ZlcmZsb3cteTogb3ZlcmxheTtcclxuXHJcbiAgLmlucHV0LWJsb2Nrcy1yb3cge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuXHJcbiAgICAuaW5wdXQtYmxvY2sge1xyXG4gICAgICBmbGV4LWJhc2lzOiA1MCU7XHJcblxyXG4gICAgICAmOmZpcnN0LWNoaWxkIHtcclxuICAgICAgICBtYXJnaW4tcmlnaHQ6IDEuNXJlbTtcclxuICAgICAgfVxyXG5cclxuICAgICAgJjpsYXN0LWNoaWxkIHtcclxuICAgICAgICBtYXJnaW4tbGVmdDogMS41cmVtO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAuY2hlY2tib3gtYmxvY2sge1xyXG4gICAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcblxyXG4gIC5wdXJjaGFzZS1zZWxlY3Qge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBiYWNrZ3JvdW5kOiB0cmFuc3BhcmVudDtcclxuICAgIGJvcmRlcjogbm9uZTtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbGluZS1oZWlnaHQ6IDEuM3JlbTtcclxuICAgIG1hcmdpbjogMS41cmVtIDAgMDtcclxuICAgIHBhZGRpbmc6IDA7XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIG1heC13aWR0aDogMTVyZW07XHJcbiAgICBoZWlnaHQ6IDEuM3JlbTtcclxuXHJcbiAgICAuYXJyb3cge1xyXG4gICAgICBtYXJnaW4tbGVmdDogMXJlbTtcclxuICAgICAgd2lkdGg6IDAuOHJlbTtcclxuICAgICAgaGVpZ2h0OiAwLjhyZW07XHJcblxyXG4gICAgICAmLmRvd24ge1xyXG4gICAgICAgIG1hc2s6IHVybCh+c3JjL2Fzc2V0cy9pY29ucy9hcnJvdy1kb3duLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi51cCB7XHJcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL2Fycm93LXVwLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLmFkZGl0aW9uYWwtZGV0YWlscyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgbWFyZ2luLXRvcDogMS41cmVtO1xyXG4gICAgcGFkZGluZzogMC41cmVtIDAgMnJlbTtcclxuXHJcbiAgICA+IGRpdiB7XHJcbiAgICAgIGZsZXgtYmFzaXM6IDI1JTtcclxuXHJcbiAgICAgICY6Zmlyc3QtY2hpbGQge1xyXG4gICAgICAgIHBhZGRpbmctbGVmdDogMS41cmVtO1xyXG4gICAgICAgIHBhZGRpbmctcmlnaHQ6IDFyZW07XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICY6bGFzdC1jaGlsZCB7XHJcbiAgICAgICAgcGFkZGluZy1sZWZ0OiAxcmVtO1xyXG4gICAgICAgIHBhZGRpbmctcmlnaHQ6IDEuNXJlbTtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnB1cmNoYXNlLXN0YXRlcyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgIGZvbnQtc2l6ZTogMS4ycmVtO1xyXG4gICAgbGluZS1oZWlnaHQ6IDIuOXJlbTtcclxuICB9XHJcblxyXG4gIC5zZW5kLWJ1dHRvbiB7XHJcbiAgICBtYXJnaW46IDIuNHJlbSAwO1xyXG4gICAgd2lkdGg6IDEwMCU7XHJcbiAgICBtYXgtd2lkdGg6IDE1cmVtO1xyXG4gIH1cclxuXHJcbiAgLnB1cmNoYXNlLWJ1dHRvbnMge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICAgIG1hcmdpbjogMi40cmVtIC0wLjVyZW07XHJcbiAgICB3aWR0aDogY2FsYygxMDAlICsgMXJlbSk7XHJcblxyXG4gICAgYnV0dG9uIHtcclxuICAgICAgZmxleDogMCAxIDMzJTtcclxuICAgICAgbWFyZ2luOiAwIDAuNXJlbTtcclxuICAgIH1cclxuICB9XHJcbn1cclxuXHJcbi5wcm9ncmVzcy1iYXItY29udGFpbmVyIHtcclxuICBwb3NpdGlvbjogYWJzb2x1dGU7XHJcbiAgYm90dG9tOiAwO1xyXG4gIGxlZnQ6IDA7XHJcbiAgcGFkZGluZzogMCAzcmVtO1xyXG4gIHdpZHRoOiAxMDAlO1xyXG4gIGhlaWdodDogM3JlbTtcclxuXHJcbiAgLnByb2dyZXNzLWJhciB7XHJcbiAgICBwb3NpdGlvbjogYWJzb2x1dGU7XHJcbiAgICB0b3A6IC0wLjdyZW07XHJcbiAgICBsZWZ0OiAwO1xyXG4gICAgbWFyZ2luOiAwIDNyZW07XHJcbiAgICB3aWR0aDogY2FsYygxMDAlIC0gNnJlbSk7XHJcbiAgICBoZWlnaHQ6IDAuN3JlbTtcclxuXHJcbiAgICAucHJvZ3Jlc3MtYmFyLWZ1bGwge1xyXG4gICAgICBoZWlnaHQ6IDAuN3JlbTtcclxuICAgIH1cclxuICB9XHJcblxyXG4gIC5wcm9ncmVzcy1sYWJlbHMge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgICBmb250LXNpemU6IDEuMnJlbTtcclxuICAgIGhlaWdodDogMTAwJTtcclxuXHJcbiAgICBzcGFuIHtcclxuICAgICAgZmxleDogMSAwIDA7XHJcbiAgICAgIHRleHQtYWxpZ246IGNlbnRlcjtcclxuXHJcbiAgICAgICY6Zmlyc3QtY2hpbGQge1xyXG4gICAgICAgIHRleHQtYWxpZ246IGxlZnQ7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICY6bGFzdC1jaGlsZCB7XHJcbiAgICAgICAgdGV4dC1hbGlnbjogcmlnaHQ7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcblxyXG4gIC5wcm9ncmVzcy10aW1lIHtcclxuICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuICAgIHRvcDogLTNyZW07XHJcbiAgICBsZWZ0OiA1MCU7XHJcbiAgICB0cmFuc2Zvcm06IHRyYW5zbGF0ZVgoLTUwJSk7XHJcbiAgICBmb250LXNpemU6IDEuMnJlbTtcclxuICB9XHJcbn1cclxuIl19 */"

/***/ }),

/***/ "./src/app/purchase/purchase.component.ts":
/*!************************************************!*\
  !*** ./src/app/purchase/purchase.component.ts ***!
  \************************************************/
/*! exports provided: PurchaseComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "PurchaseComponent", function() { return PurchaseComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
/* harmony import */ var _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! ../_helpers/pipes/int-to-money.pipe */ "./src/app/_helpers/pipes/int-to-money.pipe.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};







var PurchaseComponent = /** @class */ (function () {
    function PurchaseComponent(route, backend, variablesService, ngZone, location, intToMoneyPipe) {
        this.route = route;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.location = location;
        this.intToMoneyPipe = intToMoneyPipe;
        this.newPurchase = false;
        this.purchaseForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormGroup"]({
            description: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required),
            seller: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required),
            amount: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](null, _angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required),
            yourDeposit: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](null, _angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required),
            sellerDeposit: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](null, _angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required),
            sameAmount: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](false),
            comment: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](''),
            fee: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]('0.01'),
            payment: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]('')
        });
        this.additionalOptions = false;
        this.currentContract = null;
    }
    PurchaseComponent.prototype.checkAndChangeHistory = function () {
        var _this = this;
        if (this.currentContract.state === 201) {
            this.historyBlock = this.variablesService.currentWallet.history.find(function (item) { return item.tx_type === 8 && item.contract[0].contract_id === _this.currentContract.contract_id && item.contract[0].is_a === _this.currentContract.is_a; });
        }
        else if (this.currentContract.state === 601) {
            this.historyBlock = this.variablesService.currentWallet.history.find(function (item) { return item.tx_type === 12 && item.contract[0].contract_id === _this.currentContract.contract_id && item.contract[0].is_a === _this.currentContract.is_a; });
        }
    };
    PurchaseComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function (params) {
            _this.currentWalletId = params['id'];
        });
        this.route.params.subscribe(function (params) {
            if (params.hasOwnProperty('id')) {
                _this.currentContract = _this.variablesService.currentWallet.getContract(params['id']);
                _this.purchaseForm.setValue({
                    description: _this.currentContract.private_detailes.t,
                    seller: _this.currentContract.private_detailes.b_addr,
                    amount: _this.intToMoneyPipe.transform(_this.currentContract.private_detailes.to_pay),
                    yourDeposit: _this.intToMoneyPipe.transform(_this.currentContract.private_detailes.a_pledge),
                    sellerDeposit: _this.intToMoneyPipe.transform(_this.currentContract.private_detailes.b_pledge),
                    sameAmount: false,
                    comment: _this.currentContract.private_detailes.c,
                    fee: '0.01',
                    payment: _this.currentContract.payment_id
                });
                _this.newPurchase = false;
                // todo original code watch height_v
                if (_this.currentContract.state === 201 && _this.currentContract.height !== 0 && (_this.variablesService.height_app - _this.currentContract.height) >= 10) {
                    _this.currentContract.state = 2;
                    _this.currentContract.is_new = true;
                    _this.variablesService.currentWallet.recountNewContracts();
                }
                else if (_this.currentContract.state === 601 && _this.currentContract.height !== 0 && (_this.variablesService.height_app - _this.currentContract.height) >= 10) {
                    _this.currentContract.state = 6;
                    _this.currentContract.is_new = true;
                    _this.variablesService.currentWallet.recountNewContracts();
                }
                if (_this.currentContract.is_new) {
                    if (_this.currentContract.is_a && _this.currentContract.state === 2) {
                        _this.currentContract.state = 120;
                    }
                    if (_this.currentContract.state === 130 && _this.currentContract.cancel_expiration_time !== 0 && _this.currentContract.cancel_expiration_time < _this.variablesService.exp_med_ts) {
                        _this.currentContract.state = 2;
                    }
                    _this.variablesService.settings.viewedContracts = (_this.variablesService.settings.viewedContracts) ? _this.variablesService.settings.viewedContracts : [];
                    var findViewedCont = false;
                    for (var j = 0; j < _this.variablesService.settings.viewedContracts.length; j++) {
                        if (_this.variablesService.settings.viewedContracts[j].contract_id === _this.currentContract.contract_id && _this.variablesService.settings.viewedContracts[j].is_a === _this.currentContract.is_a) {
                            _this.variablesService.settings.viewedContracts[j].state = _this.currentContract.state;
                            findViewedCont = true;
                            break;
                        }
                    }
                    if (!findViewedCont) {
                        _this.variablesService.settings.viewedContracts.push({
                            contract_id: _this.currentContract.contract_id,
                            is_a: _this.currentContract.is_a,
                            state: _this.currentContract.state
                        });
                    }
                    _this.currentContract.is_new = false;
                    // todo need remove timeout
                    setTimeout(function () {
                        _this.variablesService.currentWallet.recountNewContracts();
                    }, 0);
                    _this.checkAndChangeHistory();
                }
            }
            else {
                _this.newPurchase = true;
            }
        });
    };
    PurchaseComponent.prototype.toggleOptions = function () {
        this.additionalOptions = !this.additionalOptions;
    };
    PurchaseComponent.prototype.getProgressBarWidth = function () {
        if (this.newPurchase) {
            return '9rem';
        }
        else {
            return '50%';
        }
    };
    PurchaseComponent.prototype.sameAmountChange = function () {
        if (this.purchaseForm.get('sameAmount').value) {
            this.purchaseForm.get('sellerDeposit').clearValidators();
            this.purchaseForm.get('sellerDeposit').updateValueAndValidity();
        }
        else {
            this.purchaseForm.get('sellerDeposit').setValidators([_angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required]);
            this.purchaseForm.get('sellerDeposit').updateValueAndValidity();
        }
    };
    PurchaseComponent.prototype.checkAddressValidation = function () {
        var _this = this;
        if (this.purchaseForm.get('seller').value) {
            this.backend.validateAddress(this.purchaseForm.get('seller').value, function (valid_status) {
                if (valid_status === false) {
                    _this.ngZone.run(function () {
                        _this.purchaseForm.get('seller').setErrors({ address_not_valid: true });
                    });
                }
            });
        }
    };
    PurchaseComponent.prototype.createPurchase = function () {
        var _this = this;
        if (this.purchaseForm.valid) {
            if (this.purchaseForm.get('sameAmount').value) {
                this.purchaseForm.get('sellerDeposit').setValue(this.purchaseForm.get('amount').value);
            }
            this.backend.createProposal(this.variablesService.currentWallet.wallet_id, this.purchaseForm.get('description').value, this.purchaseForm.get('comment').value, this.variablesService.currentWallet.address, this.purchaseForm.get('seller').value, this.purchaseForm.get('amount').value, this.purchaseForm.get('yourDeposit').value, this.purchaseForm.get('sellerDeposit').value, 12, this.purchaseForm.get('payment').value, function () {
                _this.back();
            });
        }
    };
    PurchaseComponent.prototype.back = function () {
        this.location.back();
    };
    PurchaseComponent.prototype.acceptState = function () {
        var _this = this;
        this.backend.acceptProposal(this.currentWalletId, this.currentContract.contract_id, function (accept_status) {
            if (accept_status) {
                alert('You have accepted the contract proposal. Please wait for the pledges to be made');
                _this.back();
            }
        });
    };
    PurchaseComponent.prototype.ignoredContract = function () {
        this.variablesService.settings.notViewedContracts = (this.variablesService.settings.notViewedContracts) ? this.variablesService.settings.notViewedContracts : [];
        var findViewedCont = false;
        for (var j = 0; j < this.variablesService.settings.notViewedContracts.length; j++) {
            if (this.variablesService.settings.notViewedContracts[j].contract_id === this.currentContract.contract_id && this.variablesService.settings.notViewedContracts[j].is_a === this.currentContract.is_a) {
                this.variablesService.settings.notViewedContracts[j].state = 110;
                this.variablesService.settings.notViewedContracts[j].time = this.currentContract.expiration_time;
                findViewedCont = true;
                break;
            }
        }
        if (!findViewedCont) {
            this.variablesService.settings.notViewedContracts.push({
                contract_id: this.currentContract.contract_id,
                is_a: this.currentContract.is_a,
                state: 110,
                time: this.currentContract.expiration_time
            });
        }
        this.currentContract.is_new = true;
        this.currentContract.state = 110;
        this.currentContract.time = this.currentContract.expiration_time;
        this.variablesService.currentWallet.recountNewContracts();
        alert('You have ignored the contract proposal');
        this.back();
    };
    PurchaseComponent.prototype.productNotGot = function () {
        var _this = this;
        this.backend.releaseProposal(this.currentWalletId, this.currentContract.contract_id, 'REL_B', function (release_status) {
            if (release_status) {
                alert('The pledges have been nullified.');
                _this.back();
            }
        });
    };
    PurchaseComponent.prototype.dealsDetailsFinish = function () {
        var _this = this;
        this.backend.releaseProposal(this.currentWalletId, this.currentContract.contract_id, 'REL_N', function (release_status) {
            if (release_status) {
                alert('The contract is complete. The payment has been sent.');
                _this.back();
            }
        });
    };
    PurchaseComponent.prototype.dealsDetailsCancel = function () {
        var _this = this;
        this.backend.requestCancelContract(this.currentWalletId, this.currentContract.contract_id, 12, function (cancel_status) {
            if (cancel_status) {
                alert('Proposal to cancel contract sent to seller');
                _this.back();
            }
        });
    };
    PurchaseComponent.prototype.dealsDetailsDontCanceling = function () {
        this.variablesService.settings.notViewedContracts = this.variablesService.settings.notViewedContracts ? this.variablesService.settings.notViewedContracts : [];
        var findViewedCont = false;
        for (var j = 0; j < this.variablesService.settings.notViewedContracts.length; j++) {
            if (this.variablesService.settings.notViewedContracts[j].contract_id === this.currentContract.contract_id && this.variablesService.settings.notViewedContracts[j].is_a === this.currentContract.is_a) {
                this.variablesService.settings.notViewedContracts[j].state = 130;
                this.variablesService.settings.notViewedContracts[j].time = this.currentContract.cancel_expiration_time;
                findViewedCont = true;
                break;
            }
        }
        if (!findViewedCont) {
            this.variablesService.settings.notViewedContracts.push({
                contract_id: this.currentContract.contract_id,
                is_a: this.currentContract.is_a,
                state: 130,
                time: this.currentContract.cancel_expiration_time
            });
        }
        this.currentContract.is_new = true;
        this.currentContract.state = 130;
        this.currentContract.time = this.currentContract.cancel_expiration_time;
        this.variablesService.currentWallet.recountNewContracts();
        alert('You have ignored the proposal to cancel the contract');
        this.back();
    };
    PurchaseComponent.prototype.dealsDetailsSellerCancel = function () {
        var _this = this;
        this.backend.acceptCancelContract(this.currentWalletId, this.currentContract.contract_id, function (accept_status) {
            if (accept_status) {
                alert('The contract is being cancelled. Please wait for the pledge to be returned');
                _this.back();
            }
        });
    };
    PurchaseComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
    };
    PurchaseComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-purchase',
            template: __webpack_require__(/*! ./purchase.component.html */ "./src/app/purchase/purchase.component.html"),
            styles: [__webpack_require__(/*! ./purchase.component.scss */ "./src/app/purchase/purchase.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_1__["ActivatedRoute"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _angular_common__WEBPACK_IMPORTED_MODULE_5__["Location"],
            _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_6__["IntToMoneyPipe"]])
    ], PurchaseComponent);
    return PurchaseComponent;
}());



/***/ }),

/***/ "./src/app/receive/receive.component.html":
/*!************************************************!*\
  !*** ./src/app/receive/receive.component.html ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"wrap-qr\">\r\n  <img src=\"{{qrImageSrc}}\" alt=\"qr-code\">\r\n  <div class=\"wrap-address\">\r\n    <div class=\"address\">{{variablesService.currentWallet.address}}</div>\r\n    <button type=\"button\" class=\"btn-copy-address\" (click)=\"copyAddress()\"></button>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/receive/receive.component.scss":
/*!************************************************!*\
  !*** ./src/app/receive/receive.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.wrap-qr {\n  display: flex;\n  flex-direction: column;\n  align-items: center; }\n\n.wrap-qr img {\n    margin: 4rem 0; }\n\n.wrap-qr .wrap-address {\n    display: flex;\n    align-items: center;\n    font-size: 1.4rem;\n    line-height: 2.7rem; }\n\n.wrap-qr .wrap-address .btn-copy-address {\n      -webkit-mask: url('copy.svg') no-repeat center;\n              mask: url('copy.svg') no-repeat center;\n      margin-left: 1.2rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvcmVjZWl2ZS9EOlxcemFuby9zcmNcXGFwcFxccmVjZWl2ZVxccmVjZWl2ZS5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLFlBQVcsRUFDWjs7QUFFRDtFQUNFLGNBQWE7RUFDYix1QkFBc0I7RUFDdEIsb0JBQW1CLEVBbUJwQjs7QUF0QkQ7SUFNSSxlQUFjLEVBQ2Y7O0FBUEg7SUFVSSxjQUFhO0lBQ2Isb0JBQW1CO0lBQ25CLGtCQUFpQjtJQUNqQixvQkFBbUIsRUFRcEI7O0FBckJIO01BZ0JNLCtDQUF1RDtjQUF2RCx1Q0FBdUQ7TUFDdkQsb0JBQW1CO01BQ25CLGNBQWE7TUFDYixlQUFjLEVBQ2YiLCJmaWxlIjoic3JjL2FwcC9yZWNlaXZlL3JlY2VpdmUuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XHJcbiAgd2lkdGg6IDEwMCU7XHJcbn1cclxuXHJcbi53cmFwLXFyIHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuXHJcbiAgaW1nIHtcclxuICAgIG1hcmdpbjogNHJlbSAwO1xyXG4gIH1cclxuXHJcbiAgLndyYXAtYWRkcmVzcyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGZvbnQtc2l6ZTogMS40cmVtO1xyXG4gICAgbGluZS1oZWlnaHQ6IDIuN3JlbTtcclxuXHJcbiAgICAuYnRuLWNvcHktYWRkcmVzcyB7XHJcbiAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvY29weS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIG1hcmdpbi1sZWZ0OiAxLjJyZW07XHJcbiAgICAgIHdpZHRoOiAxLjdyZW07XHJcbiAgICAgIGhlaWdodDogMS43cmVtO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

/***/ }),

/***/ "./src/app/receive/receive.component.ts":
/*!**********************************************!*\
  !*** ./src/app/receive/receive.component.ts ***!
  \**********************************************/
/*! exports provided: ReceiveComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ReceiveComponent", function() { return ReceiveComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var qrcode__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! qrcode */ "./node_modules/qrcode/lib/browser.js");
/* harmony import */ var qrcode__WEBPACK_IMPORTED_MODULE_1___default = /*#__PURE__*/__webpack_require__.n(qrcode__WEBPACK_IMPORTED_MODULE_1__);
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};





var ReceiveComponent = /** @class */ (function () {
    function ReceiveComponent(route, backend, variablesService) {
        this.route = route;
        this.backend = backend;
        this.variablesService = variablesService;
    }
    ReceiveComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function () {
            qrcode__WEBPACK_IMPORTED_MODULE_1___default.a.toDataURL(_this.variablesService.currentWallet.address, {
                width: 106,
                height: 106
            }).then(function (url) {
                _this.qrImageSrc = url;
            }).catch(function (err) {
                console.error(err);
            });
        });
    };
    ReceiveComponent.prototype.copyAddress = function () {
        this.backend.setClipboard(this.variablesService.currentWallet.address);
    };
    ReceiveComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
    };
    ReceiveComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-receive',
            template: __webpack_require__(/*! ./receive.component.html */ "./src/app/receive/receive.component.html"),
            styles: [__webpack_require__(/*! ./receive.component.scss */ "./src/app/receive/receive.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_4__["ActivatedRoute"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"]])
    ], ReceiveComponent);
    return ReceiveComponent;
}());



/***/ }),

/***/ "./src/app/restore-wallet/restore-wallet.component.html":
/*!**************************************************************!*\
  !*** ./src/app/restore-wallet/restore-wallet.component.html ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span>{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\r\n      <span>{{ 'BREADCRUMBS.RESTORE_WALLET' | translate }}</span>\r\n    </div>\r\n    <a class=\"back-btn\" [routerLink]=\"['/main']\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </a>\r\n  </div>\r\n\r\n  <form class=\"form-restore\" [formGroup]=\"restoreForm\">\r\n    <div class=\"input-block half-block\">\r\n      <label for=\"wallet-name\">{{ 'RESTORE_WALLET.LABEL_NAME' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [attr.disabled]=\"walletSaved ? '' : null\">\r\n    </div>\r\n    <div class=\"error-block half-block\" *ngIf=\"restoreForm.controls['name'].invalid && (restoreForm.controls['name'].dirty || restoreForm.controls['name'].touched)\">\r\n      <div *ngIf=\"restoreForm.controls['name'].errors['required']\">\r\n        Name is required.\r\n      </div>\r\n      <div *ngIf=\"restoreForm.controls['name'].errors['duplicate']\">\r\n        Name is duplicate.\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block half-block\">\r\n      <label for=\"wallet-password\">{{ 'RESTORE_WALLET.PASS' | translate }}</label>\r\n      <input type=\"password\" id=\"wallet-password\" formControlName=\"password\" [attr.disabled]=\"walletSaved ? '' : null\">\r\n    </div>\r\n    <div class=\"input-block half-block\">\r\n      <label for=\"confirm-wallet-password\">{{ 'RESTORE_WALLET.CONFIRM' | translate }}</label>\r\n      <input type=\"password\" id=\"confirm-wallet-password\" formControlName=\"confirm\" [attr.disabled]=\"walletSaved ? '' : null\">\r\n    </div>\r\n    <div class=\"error-block half-block\" *ngIf=\"restoreForm.controls['password'].dirty && restoreForm.controls['confirm'].dirty && restoreForm.errors\">\r\n      <div *ngIf=\"restoreForm.errors['confirm_mismatch']\">\r\n        Confirm password not match.\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"phrase-key\">{{ 'RESTORE_WALLET.LABEL_PHRASE_KEY' | translate }}</label>\r\n      <input type=\"text\" id=\"phrase-key\" formControlName=\"key\" [attr.disabled]=\"walletSaved ? '' : null\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"restoreForm.controls['key'].invalid && (restoreForm.controls['key'].dirty || restoreForm.controls['key'].touched)\">\r\n      <div *ngIf=\"restoreForm.controls['key'].errors['required']\">\r\n        Key is required.\r\n      </div>\r\n      <div *ngIf=\"restoreForm.controls['key'].errors['key_not_valid']\">\r\n        Key not valid.\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"wrap-buttons\">\r\n      <button type=\"button\" class=\"transparent-button\" *ngIf=\"walletSaved\">{{restoreForm.controls['name'].value}}</button>\r\n      <button type=\"button\" class=\"blue-button select-button\" (click)=\"saveWallet()\" [disabled]=\"!restoreForm.valid\" *ngIf=\"!walletSaved\">{{ 'RESTORE_WALLET.BUTTON_SELECT' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button create-button\" (click)=\"createWallet()\" [disabled]=\"!walletSaved\">{{ 'RESTORE_WALLET.BUTTON_CREATE' | translate }}</button>\r\n    </div>\r\n  </form>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/restore-wallet/restore-wallet.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/restore-wallet/restore-wallet.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-restore {\n  margin: 2.4rem 0;\n  width: 100%; }\n  .form-restore .input-block.half-block, .form-restore .error-block.half-block {\n    width: 50%; }\n  .form-restore .wrap-buttons {\n    display: flex;\n    margin: 2.5rem -0.7rem;\n    width: 50%; }\n  .form-restore .wrap-buttons button {\n      margin: 0 0.7rem; }\n  .form-restore .wrap-buttons button.transparent-button {\n        flex-basis: 50%; }\n  .form-restore .wrap-buttons button.select-button {\n        flex-basis: 60%; }\n  .form-restore .wrap-buttons button.create-button {\n        flex: 1 1 50%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvcmVzdG9yZS13YWxsZXQvRDpcXHphbm8vc3JjXFxhcHBcXHJlc3RvcmUtd2FsbGV0XFxyZXN0b3JlLXdhbGxldC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGlCQUFnQjtFQUNoQixZQUFXLEVBOEJaO0VBaENEO0lBT00sV0FBVSxFQUNYO0VBUkw7SUFZSSxjQUFhO0lBQ2IsdUJBQXNCO0lBQ3RCLFdBQVUsRUFpQlg7RUEvQkg7TUFpQk0saUJBQWdCLEVBYWpCO0VBOUJMO1FBb0JRLGdCQUFlLEVBQ2hCO0VBckJQO1FBd0JRLGdCQUFlLEVBQ2hCO0VBekJQO1FBNEJRLGNBQWEsRUFDZCIsImZpbGUiOiJzcmMvYXBwL3Jlc3RvcmUtd2FsbGV0L3Jlc3RvcmUtd2FsbGV0LmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tcmVzdG9yZSB7XHJcbiAgbWFyZ2luOiAyLjRyZW0gMDtcclxuICB3aWR0aDogMTAwJTtcclxuXHJcbiAgLmlucHV0LWJsb2NrLCAuZXJyb3ItYmxvY2sge1xyXG5cclxuICAgICYuaGFsZi1ibG9jayB7XHJcbiAgICAgIHdpZHRoOiA1MCU7XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAud3JhcC1idXR0b25zIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBtYXJnaW46IDIuNXJlbSAtMC43cmVtO1xyXG4gICAgd2lkdGg6IDUwJTtcclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBtYXJnaW46IDAgMC43cmVtO1xyXG5cclxuICAgICAgJi50cmFuc3BhcmVudC1idXR0b24ge1xyXG4gICAgICAgIGZsZXgtYmFzaXM6IDUwJTtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi5zZWxlY3QtYnV0dG9uIHtcclxuICAgICAgICBmbGV4LWJhc2lzOiA2MCU7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYuY3JlYXRlLWJ1dHRvbiB7XHJcbiAgICAgICAgZmxleDogMSAxIDUwJTtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

/***/ }),

/***/ "./src/app/restore-wallet/restore-wallet.component.ts":
/*!************************************************************!*\
  !*** ./src/app/restore-wallet/restore-wallet.component.ts ***!
  \************************************************************/
/*! exports provided: RestoreWalletComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "RestoreWalletComponent", function() { return RestoreWalletComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/models/wallet.model */ "./src/app/_helpers/models/wallet.model.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var RestoreWalletComponent = /** @class */ (function () {
    function RestoreWalletComponent(router, backend, variablesService, ngZone) {
        var _this = this;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.restoreForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            name: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [_angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required, function (g) {
                    for (var i = 0; i < _this.variablesService.wallets.length; i++) {
                        if (g.value === _this.variablesService.wallets[i].name) {
                            return { 'duplicate': true };
                        }
                    }
                    return null;
                }]),
            key: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required),
            password: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](''),
            confirm: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('')
        }, function (g) {
            return g.get('password').value === g.get('confirm').value ? null : { 'confirm_mismatch': true };
        });
        this.wallet = {
            id: ''
        };
        this.walletSaved = false;
    }
    RestoreWalletComponent.prototype.ngOnInit = function () {
    };
    RestoreWalletComponent.prototype.createWallet = function () {
        this.router.navigate(['/seed-phrase'], { queryParams: { wallet_id: this.wallet.id } });
    };
    RestoreWalletComponent.prototype.saveWallet = function () {
        var _this = this;
        if (this.restoreForm.valid) {
            this.backend.isValidRestoreWalletText(this.restoreForm.get('key').value, function (valid_status, valid_data) {
                if (valid_data === 'FALSE') {
                    _this.ngZone.run(function () {
                        _this.restoreForm.get('key').setErrors({ key_not_valid: true });
                    });
                }
                else {
                    _this.backend.saveFileDialog('Save the wallet file.', '*', _this.variablesService.settings.default_path, function (save_status, save_data) {
                        if (save_status) {
                            _this.variablesService.settings.default_path = save_data.path.substr(0, save_data.path.lastIndexOf('/'));
                            _this.backend.restoreWallet(save_data.path, _this.restoreForm.get('password').value, _this.restoreForm.get('key').value, function (restore_status, restore_data) {
                                if (restore_status) {
                                    _this.wallet.id = restore_data.wallet_id;
                                    _this.variablesService.opening_wallet = new _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_5__["Wallet"](restore_data.wallet_id, _this.restoreForm.get('name').value, _this.restoreForm.get('password').value, restore_data['wi'].path, restore_data['wi'].address, restore_data['wi'].balance, restore_data['wi'].unlocked_balance, restore_data['wi'].mined_total, restore_data['wi'].tracking_hey);
                                    if (restore_data.recent_history && restore_data.recent_history.history) {
                                        _this.variablesService.opening_wallet.prepareHistory(restore_data.recent_history.history);
                                    }
                                    _this.backend.getContracts(_this.variablesService.opening_wallet.wallet_id, function (contracts_status, contracts_data) {
                                        if (contracts_status && contracts_data.hasOwnProperty('contracts')) {
                                            _this.ngZone.run(function () {
                                                _this.variablesService.opening_wallet.prepareContractsAfterOpen(contracts_data.contracts, _this.variablesService.exp_med_ts, _this.variablesService.height_app, _this.variablesService.settings.viewedContracts, _this.variablesService.settings.notViewedContracts);
                                            });
                                        }
                                    });
                                    _this.ngZone.run(function () {
                                        _this.walletSaved = true;
                                    });
                                }
                                else {
                                    alert('SAFES.NOT_CORRECT_FILE_OR_PASSWORD');
                                }
                            });
                        }
                    });
                }
            });
        }
    };
    RestoreWalletComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-restore-wallet',
            template: __webpack_require__(/*! ./restore-wallet.component.html */ "./src/app/restore-wallet/restore-wallet.component.html"),
            styles: [__webpack_require__(/*! ./restore-wallet.component.scss */ "./src/app/restore-wallet/restore-wallet.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_2__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], RestoreWalletComponent);
    return RestoreWalletComponent;
}());



/***/ }),

/***/ "./src/app/seed-phrase/seed-phrase.component.html":
/*!********************************************************!*\
  !*** ./src/app/seed-phrase/seed-phrase.component.html ***!
  \********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span>{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\r\n      <span>{{ 'BREADCRUMBS.SAVE_PHRASE' | translate }}</span>\r\n    </div>\r\n    <a class=\"back-btn\" [routerLink]=\"['/main']\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </a>\r\n  </div>\r\n\r\n  <h3 class=\"seed-phrase-title\">{{ 'SEED_PHRASE.TITLE' | translate }}</h3>\r\n\r\n  <div class=\"seed-phrase-content\">\r\n    <ng-container *ngFor=\"let word of seedPhrase.split(' '); let index = index\">\r\n      <div class=\"word\">{{(index + 1) + '. ' + word}}</div>\r\n    </ng-container>\r\n  </div>\r\n\r\n  <button type=\"button\" class=\"blue-button\" (click)=\"runWallet()\">{{ 'SEED_PHRASE.BUTTON_CREATE_ACCOUNT' | translate }}</button>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/seed-phrase/seed-phrase.component.scss":
/*!********************************************************!*\
  !*** ./src/app/seed-phrase/seed-phrase.component.scss ***!
  \********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".seed-phrase-title {\n  line-height: 2.2rem;\n  padding: 2.2rem 0; }\n\n.seed-phrase-content {\n  display: flex;\n  flex-direction: column;\n  flex-wrap: wrap;\n  padding: 1.4rem;\n  width: 100%;\n  height: 12rem; }\n\n.seed-phrase-content .word {\n    line-height: 2.2rem;\n    max-width: 13rem; }\n\nbutton {\n  margin: 2.8rem 0;\n  width: 25%;\n  min-width: 1.5rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvc2VlZC1waHJhc2UvRDpcXHphbm8vc3JjXFxhcHBcXHNlZWQtcGhyYXNlXFxzZWVkLXBocmFzZS5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLG9CQUFtQjtFQUNuQixrQkFBaUIsRUFDbEI7O0FBRUQ7RUFDRSxjQUFhO0VBQ2IsdUJBQXNCO0VBQ3RCLGdCQUFlO0VBQ2YsZ0JBQWU7RUFDZixZQUFXO0VBQ1gsY0FBYSxFQU1kOztBQVpEO0lBU0ksb0JBQW1CO0lBQ25CLGlCQUFnQixFQUNqQjs7QUFHSDtFQUNFLGlCQUFnQjtFQUNoQixXQUFVO0VBQ1Ysa0JBQWlCLEVBQ2xCIiwiZmlsZSI6InNyYy9hcHAvc2VlZC1waHJhc2Uvc2VlZC1waHJhc2UuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyIuc2VlZC1waHJhc2UtdGl0bGUge1xyXG4gIGxpbmUtaGVpZ2h0OiAyLjJyZW07XHJcbiAgcGFkZGluZzogMi4ycmVtIDA7XHJcbn1cclxuXHJcbi5zZWVkLXBocmFzZS1jb250ZW50IHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgZmxleC13cmFwOiB3cmFwO1xyXG4gIHBhZGRpbmc6IDEuNHJlbTtcclxuICB3aWR0aDogMTAwJTtcclxuICBoZWlnaHQ6IDEycmVtO1xyXG5cclxuICAud29yZCB7XHJcbiAgICBsaW5lLWhlaWdodDogMi4ycmVtO1xyXG4gICAgbWF4LXdpZHRoOiAxM3JlbTtcclxuICB9XHJcbn1cclxuXHJcbmJ1dHRvbiB7XHJcbiAgbWFyZ2luOiAyLjhyZW0gMDtcclxuICB3aWR0aDogMjUlO1xyXG4gIG1pbi13aWR0aDogMS41cmVtO1xyXG59XHJcbiJdfQ== */"

/***/ }),

/***/ "./src/app/seed-phrase/seed-phrase.component.ts":
/*!******************************************************!*\
  !*** ./src/app/seed-phrase/seed-phrase.component.ts ***!
  \******************************************************/
/*! exports provided: SeedPhraseComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "SeedPhraseComponent", function() { return SeedPhraseComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};




var SeedPhraseComponent = /** @class */ (function () {
    function SeedPhraseComponent(route, router, backend, variablesService, ngZone) {
        this.route = route;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.seedPhrase = '';
    }
    SeedPhraseComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.route.queryParams.subscribe(function (params) {
            if (params.wallet_id) {
                _this.wallet_id = params.wallet_id;
                _this.backend.getSmartSafeInfo(params.wallet_id, function (status, data) {
                    if (data.hasOwnProperty('restore_key')) {
                        _this.ngZone.run(function () {
                            _this.seedPhrase = data['restore_key'].trim();
                        });
                    }
                });
            }
        });
    };
    SeedPhraseComponent.prototype.runWallet = function () {
        var _this = this;
        var exists = false;
        this.variablesService.wallets.forEach(function (wallet) {
            if (wallet.address === _this.variablesService.opening_wallet.address) {
                exists = true;
            }
        });
        if (!exists) {
            this.backend.runWallet(this.wallet_id, function (run_status, run_data) {
                if (run_status) {
                    _this.variablesService.wallets.push(_this.variablesService.opening_wallet);
                    _this.backend.storeSecureAppData(function (status, data) {
                        console.log('Store App Data', status, data);
                    });
                    _this.ngZone.run(function () {
                        _this.router.navigate(['/wallet/' + _this.wallet_id]);
                    });
                }
                else {
                    console.log(run_data['error_code']);
                }
                // $rootScope.reloadCounters();
                // $rootScope.$broadcast('NEED_REFRESH_HISTORY');
                // $rootScope.saveSecureData();
            });
        }
        else {
            this.variablesService.opening_wallet = null;
            this.backend.closeWallet(this.wallet_id, function (close_status, close_data) {
                console.log(close_status, close_data);
                _this.ngZone.run(function () {
                    _this.router.navigate(['/']);
                });
            });
        }
    };
    SeedPhraseComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-seed-phrase',
            template: __webpack_require__(/*! ./seed-phrase.component.html */ "./src/app/seed-phrase/seed-phrase.component.html"),
            styles: [__webpack_require__(/*! ./seed-phrase.component.scss */ "./src/app/seed-phrase/seed-phrase.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_2__["ActivatedRoute"],
            _angular_router__WEBPACK_IMPORTED_MODULE_2__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_1__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], SeedPhraseComponent);
    return SeedPhraseComponent;
}());



/***/ }),

/***/ "./src/app/send/send.component.html":
/*!******************************************!*\
  !*** ./src/app/send/send.component.html ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<form class=\"form-send\" [formGroup]=\"sendForm\" (ngSubmit)=\"onSend()\">\r\n\r\n  <div class=\"input-block\">\r\n    <label for=\"send-address\">{{ 'SEND.ADDRESS' | translate }}</label>\r\n    <input type=\"text\" id=\"send-address\" formControlName=\"address\" (blur)=\"checkAddressValidation()\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n  </div>\r\n\r\n  <div class=\"error-block\" *ngIf=\"sendForm.controls['address'].invalid && (sendForm.controls['address'].dirty || sendForm.controls['address'].touched)\">\r\n    <div *ngIf=\"sendForm.controls['address'].errors['required']\">\r\n      Address is required.\r\n    </div>\r\n    <div *ngIf=\"sendForm.controls['address'].errors['address_not_valid']\">\r\n      Address not valid.\r\n    </div>\r\n  </div>\r\n\r\n  <div class=\"input-blocks-row\">\r\n    <div class=\"input-block\">\r\n      <label for=\"send-amount\">{{ 'SEND.AMOUNT' | translate }}</label>\r\n      <input type=\"text\" id=\"send-amount\" formControlName=\"amount\" appInputValidate=\"money\" maxlength=\"21\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n\r\n    <div class=\"error-block\" *ngIf=\"sendForm.controls['amount'].invalid && (sendForm.controls['amount'].dirty || sendForm.controls['amount'].touched)\">\r\n      <div *ngIf=\"sendForm.controls['amount'].errors['required']\">\r\n        Amount is required.\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"send-comment\">{{ 'SEND.COMMENT' | translate }}</label>\r\n      <input type=\"text\" id=\"send-comment\" formControlName=\"comment\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n  </div>\r\n\r\n  <button type=\"button\" class=\"send-select\" (click)=\"toggleOptions()\">\r\n    <span>{{ 'SEND.DETAILS' | translate }}</span><i class=\"icon arrow\" [class.down]=\"!additionalOptions\" [class.up]=\"additionalOptions\"></i>\r\n  </button>\r\n\r\n  <div class=\"additional-details\" *ngIf=\"additionalOptions\">\r\n    <div class=\"input-block\">\r\n      <label for=\"send-mixin\">{{ 'SEND.MIXIN' | translate }}</label>\r\n      <input type=\"text\" id=\"send-mixin\" formControlName=\"mixin\" appInputValidate=\"integer\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"sendForm.controls['mixin'].invalid && (sendForm.controls['mixin'].dirty || sendForm.controls['mixin'].touched)\">\r\n      <div *ngIf=\"sendForm.controls['mixin'].errors['required']\">\r\n        Amount is required.\r\n      </div>\r\n    </div>\r\n    <div class=\"input-block\">\r\n      <label for=\"send-fee\">{{ 'SEND.FEE' | translate }}</label>\r\n      <input type=\"text\" id=\"send-fee\" formControlName=\"fee\" appInputValidate=\"money\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"sendForm.controls['fee'].invalid && (sendForm.controls['fee'].dirty || sendForm.controls['fee'].touched)\">\r\n      <div *ngIf=\"sendForm.controls['fee'].errors['required']\">\r\n        Amount is required.\r\n      </div>\r\n    </div>\r\n  </div>\r\n  <button type=\"submit\" class=\"blue-button\" [disabled]=\"!sendForm.valid || !variablesService.currentWallet.loaded\">{{ 'SEND.BUTTON' | translate }}</button>\r\n</form>\r\n"

/***/ }),

/***/ "./src/app/send/send.component.scss":
/*!******************************************!*\
  !*** ./src/app/send/send.component.scss ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.form-send .input-blocks-row {\n  display: flex; }\n\n.form-send .input-blocks-row > div {\n    flex-basis: 50%; }\n\n.form-send .input-blocks-row > div:first-child {\n      margin-right: 1.5rem; }\n\n.form-send .input-blocks-row > div:last-child {\n      margin-left: 1.5rem; }\n\n.form-send .send-select {\n  display: flex;\n  align-items: center;\n  background: transparent;\n  border: none;\n  font-size: 1.3rem;\n  line-height: 1.3rem;\n  margin: 1.5rem 0 0;\n  padding: 0;\n  width: 100%;\n  max-width: 15rem;\n  height: 1.3rem; }\n\n.form-send .send-select .arrow {\n    margin-left: 1rem;\n    width: 0.8rem;\n    height: 0.8rem; }\n\n.form-send .send-select .arrow.down {\n      -webkit-mask: url('arrow-down.svg') no-repeat center;\n              mask: url('arrow-down.svg') no-repeat center; }\n\n.form-send .send-select .arrow.up {\n      -webkit-mask: url('arrow-up.svg') no-repeat center;\n              mask: url('arrow-up.svg') no-repeat center; }\n\n.form-send .additional-details {\n  display: flex;\n  margin-top: 1.5rem;\n  padding: 0.5rem 0 2rem; }\n\n.form-send .additional-details > div {\n    flex-basis: 25%; }\n\n.form-send .additional-details > div:first-child {\n      padding-left: 1.5rem;\n      padding-right: 1rem; }\n\n.form-send .additional-details > div:last-child {\n      padding-left: 1rem;\n      padding-right: 1.5rem; }\n\n.form-send button {\n  margin: 2.4rem 0;\n  width: 100%;\n  max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvc2VuZC9EOlxcemFuby9zcmNcXGFwcFxcc2VuZFxcc2VuZC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLFlBQVcsRUFDWjs7QUFFRDtFQUdJLGNBQWEsRUFhZDs7QUFoQkg7SUFNTSxnQkFBZSxFQVNoQjs7QUFmTDtNQVNRLHFCQUFvQixFQUNyQjs7QUFWUDtNQWFRLG9CQUFtQixFQUNwQjs7QUFkUDtFQW1CSSxjQUFhO0VBQ2Isb0JBQW1CO0VBQ25CLHdCQUF1QjtFQUN2QixhQUFZO0VBQ1osa0JBQWlCO0VBQ2pCLG9CQUFtQjtFQUNuQixtQkFBa0I7RUFDbEIsV0FBVTtFQUNWLFlBQVc7RUFDWCxpQkFBZ0I7RUFDaEIsZUFBYyxFQWVmOztBQTVDSDtJQWdDTSxrQkFBaUI7SUFDakIsY0FBYTtJQUNiLGVBQWMsRUFTZjs7QUEzQ0w7TUFxQ1EscURBQTREO2NBQTVELDZDQUE0RCxFQUM3RDs7QUF0Q1A7TUF5Q1EsbURBQTBEO2NBQTFELDJDQUEwRCxFQUMzRDs7QUExQ1A7RUErQ0ksY0FBYTtFQUNiLG1CQUFrQjtFQUNsQix1QkFBc0IsRUFldkI7O0FBaEVIO0lBb0RNLGdCQUFlLEVBV2hCOztBQS9ETDtNQXVEUSxxQkFBb0I7TUFDcEIsb0JBQW1CLEVBQ3BCOztBQXpEUDtNQTREUSxtQkFBa0I7TUFDbEIsc0JBQXFCLEVBQ3RCOztBQTlEUDtFQW1FSSxpQkFBZ0I7RUFDaEIsWUFBVztFQUNYLGlCQUFnQixFQUNqQiIsImZpbGUiOiJzcmMvYXBwL3NlbmQvc2VuZC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICB3aWR0aDogMTAwJTtcclxufVxyXG5cclxuLmZvcm0tc2VuZCB7XHJcblxyXG4gIC5pbnB1dC1ibG9ja3Mtcm93IHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcblxyXG4gICAgPiBkaXYge1xyXG4gICAgICBmbGV4LWJhc2lzOiA1MCU7XHJcblxyXG4gICAgICAmOmZpcnN0LWNoaWxkIHtcclxuICAgICAgICBtYXJnaW4tcmlnaHQ6IDEuNXJlbTtcclxuICAgICAgfVxyXG5cclxuICAgICAgJjpsYXN0LWNoaWxkIHtcclxuICAgICAgICBtYXJnaW4tbGVmdDogMS41cmVtO1xyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAuc2VuZC1zZWxlY3Qge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBiYWNrZ3JvdW5kOiB0cmFuc3BhcmVudDtcclxuICAgIGJvcmRlcjogbm9uZTtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbGluZS1oZWlnaHQ6IDEuM3JlbTtcclxuICAgIG1hcmdpbjogMS41cmVtIDAgMDtcclxuICAgIHBhZGRpbmc6IDA7XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIG1heC13aWR0aDogMTVyZW07XHJcbiAgICBoZWlnaHQ6IDEuM3JlbTtcclxuXHJcbiAgICAuYXJyb3cge1xyXG4gICAgICBtYXJnaW4tbGVmdDogMXJlbTtcclxuICAgICAgd2lkdGg6IDAuOHJlbTtcclxuICAgICAgaGVpZ2h0OiAwLjhyZW07XHJcblxyXG4gICAgICAmLmRvd24ge1xyXG4gICAgICAgIG1hc2s6IHVybCh+c3JjL2Fzc2V0cy9pY29ucy9hcnJvdy1kb3duLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi51cCB7XHJcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL2Fycm93LXVwLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLmFkZGl0aW9uYWwtZGV0YWlscyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgbWFyZ2luLXRvcDogMS41cmVtO1xyXG4gICAgcGFkZGluZzogMC41cmVtIDAgMnJlbTtcclxuXHJcbiAgICA+IGRpdiB7XHJcbiAgICAgIGZsZXgtYmFzaXM6IDI1JTtcclxuXHJcbiAgICAgICY6Zmlyc3QtY2hpbGQge1xyXG4gICAgICAgIHBhZGRpbmctbGVmdDogMS41cmVtO1xyXG4gICAgICAgIHBhZGRpbmctcmlnaHQ6IDFyZW07XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICY6bGFzdC1jaGlsZCB7XHJcbiAgICAgICAgcGFkZGluZy1sZWZ0OiAxcmVtO1xyXG4gICAgICAgIHBhZGRpbmctcmlnaHQ6IDEuNXJlbTtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgYnV0dG9uIHtcclxuICAgIG1hcmdpbjogMi40cmVtIDA7XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIG1heC13aWR0aDogMTVyZW07XHJcbiAgfVxyXG59XHJcbiJdfQ== */"

/***/ }),

/***/ "./src/app/send/send.component.ts":
/*!****************************************!*\
  !*** ./src/app/send/send.component.ts ***!
  \****************************************/
/*! exports provided: SendComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "SendComponent", function() { return SendComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};





var SendComponent = /** @class */ (function () {
    function SendComponent(route, backend, variablesService, ngZone) {
        this.route = route;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.currentWalletId = null;
        this.sendForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            address: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required),
            amount: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](null, _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required),
            comment: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](''),
            mixin: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](0, _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required),
            fee: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('0.01', _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required)
        });
        this.additionalOptions = false;
    }
    SendComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function (params) {
            _this.currentWalletId = params['id'];
            _this.sendForm.reset({ address: '', amount: null, comment: '', mixin: 0, fee: '0.01' });
        });
    };
    SendComponent.prototype.checkAddressValidation = function () {
        var _this = this;
        if (this.sendForm.get('address').value) {
            this.backend.validateAddress(this.sendForm.get('address').value, function (valid_status) {
                if (valid_status === false) {
                    _this.ngZone.run(function () {
                        _this.sendForm.get('address').setErrors({ address_not_valid: true });
                    });
                }
            });
        }
    };
    SendComponent.prototype.onSend = function () {
        var _this = this;
        if (this.sendForm.valid) {
            this.backend.validateAddress(this.sendForm.get('address').value, function (valid_status) {
                if (valid_status === false) {
                    _this.ngZone.run(function () {
                        _this.sendForm.get('address').setErrors({ address_not_valid: true });
                    });
                }
                else {
                    _this.backend.sendMoney(_this.currentWalletId, _this.sendForm.get('address').value, _this.sendForm.get('amount').value, _this.sendForm.get('fee').value, _this.sendForm.get('mixin').value, _this.sendForm.get('comment').value, function (send_status, send_data) {
                        if (send_status) {
                            alert('SEND_MONEY.SUCCESS_SENT');
                            _this.sendForm.reset({ address: '', amount: null, comment: '', mixin: 0, fee: '0.01' });
                        }
                    });
                }
            });
        }
    };
    SendComponent.prototype.toggleOptions = function () {
        this.additionalOptions = !this.additionalOptions;
    };
    SendComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
    };
    SendComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-send',
            template: __webpack_require__(/*! ./send.component.html */ "./src/app/send/send.component.html"),
            styles: [__webpack_require__(/*! ./send.component.scss */ "./src/app/send/send.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_2__["ActivatedRoute"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], SendComponent);
    return SendComponent;
}());



/***/ }),

/***/ "./src/app/settings/settings.component.html":
/*!**************************************************!*\
  !*** ./src/app/settings/settings.component.html ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\r\n  <div class=\"head\">\r\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <h3 class=\"settings-title\">{{ 'SETTINGS.TITLE' | translate }}</h3>\r\n  <div class=\"theme-selection\">\r\n    <div class=\"radio-block\">\r\n      <input class=\"style-radio\" type=\"radio\" id=\"dark\" name=\"theme\" value=\"dark\" [checked]=\"theme == 'dark'\" (change)=\"setTheme('dark')\">\r\n      <label for=\"dark\">{{ 'SETTINGS.DARK_THEME' | translate }}</label>\r\n    </div>\r\n    <div class=\"radio-block\">\r\n      <input class=\"style-radio\" type=\"radio\" id=\"white\" name=\"theme\" value=\"white\" [checked]=\"theme == 'white'\" (change)=\"setTheme('white')\">\r\n      <label for=\"white\">{{ 'SETTINGS.WHITE_THEME' | translate }}</label>\r\n    </div>\r\n    <div class=\"radio-block\">\r\n      <input class=\"style-radio\" type=\"radio\" id=\"gray\" name=\"theme\" value=\"gray\" [checked]=\"theme == 'gray'\" (change)=\"setTheme('gray')\">\r\n      <label for=\"gray\">{{ 'SETTINGS.GRAY_THEME' | translate }}</label>\r\n    </div>\r\n  </div>\r\n\r\n  <form class=\"master-password\" [formGroup]=\"changeForm\" (ngSubmit)=\"onSubmitChangePass()\">\r\n    <span class=\"master-password-title\">{{ 'SETTINGS.MASTER_PASSWORD.TITLE' | translate }}</span>\r\n    <div class=\"input-block\">\r\n      <label for=\"old-password\">{{ 'SETTINGS.MASTER_PASSWORD.OLD' | translate }}</label>\r\n      <input type=\"password\" id=\"old-password\" formControlName=\"password\"/>\r\n      <div *ngIf=\"changeForm.controls['password'].invalid && (changeForm.controls['password'].dirty || changeForm.controls['password'].touched)\">\r\n        <div *ngIf=\"changeForm.controls['password'].errors.required\">\r\n          Password is required.\r\n        </div>\r\n      </div>\r\n      <div *ngIf=\"changeForm.invalid && changeForm.controls['password'].valid && (changeForm.controls['password'].dirty || changeForm.controls['password'].touched) && changeForm.errors && changeForm.errors.pass_mismatch\">\r\n        Old password not match.\r\n      </div>\r\n    </div>\r\n    <div class=\"input-block\">\r\n      <label for=\"new-password\">{{ 'SETTINGS.MASTER_PASSWORD.NEW' | translate }}</label>\r\n      <input type=\"password\" id=\"new-password\" formControlName=\"new_password\"/>\r\n      <div *ngIf=\"changeForm.controls['new_password'].invalid && (changeForm.controls['new_password'].dirty || changeForm.controls['new_password'].touched)\">\r\n        <div *ngIf=\"changeForm.controls['new_password'].errors.required\">\r\n          Password is required.\r\n        </div>\r\n      </div>\r\n    </div>\r\n    <div class=\"input-block\">\r\n      <label for=\"confirm-password\">{{ 'SETTINGS.MASTER_PASSWORD.CONFIRM' | translate }}</label>\r\n      <input type=\"password\" id=\"confirm-password\" formControlName=\"new_confirmation\"/>\r\n      <div *ngIf=\"changeForm.controls['new_confirmation'].invalid && (changeForm.controls['new_confirmation'].dirty || changeForm.controls['new_confirmation'].touched)\">\r\n        <div *ngIf=\"changeForm.controls['new_confirmation'].errors.required\">\r\n          Password is required.\r\n        </div>\r\n      </div>\r\n      <div *ngIf=\"changeForm.invalid && (changeForm.controls['new_confirmation'].dirty || changeForm.controls['new_confirmation'].touched) && changeForm.errors && changeForm.errors.confirm_mismatch\">\r\n        Confirm password not match.\r\n      </div>\r\n    </div>\r\n    <button type=\"submit\" class=\"blue-button\">{{ 'SETTINGS.MASTER_PASSWORD.BUTTON' | translate }}</button>\r\n  </form>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/settings/settings.component.scss":
/*!**************************************************!*\
  !*** ./src/app/settings/settings.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".head {\n  justify-content: flex-end; }\n\n.settings-title {\n  font-size: 1.7rem; }\n\n.theme-selection {\n  display: flex;\n  flex-direction: column;\n  align-items: flex-start;\n  margin: 2.4rem 0;\n  width: 50%; }\n\n.theme-selection .radio-block {\n    display: flex;\n    align-items: center;\n    justify-content: flex-start;\n    font-size: 1.3rem;\n    line-height: 2.7rem; }\n\n.master-password {\n  width: 50%; }\n\n.master-password .master-password-title {\n    display: flex;\n    font-size: 1.5rem;\n    line-height: 2.7rem;\n    margin-bottom: 1rem; }\n\n.master-password button {\n    margin: 2.5rem auto;\n    width: 100%;\n    max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvc2V0dGluZ3MvRDpcXHphbm8vc3JjXFxhcHBcXHNldHRpbmdzXFxzZXR0aW5ncy5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLDBCQUF5QixFQUMxQjs7QUFFRDtFQUNFLGtCQUFpQixFQUNsQjs7QUFFRDtFQUNFLGNBQWE7RUFDYix1QkFBc0I7RUFDdEIsd0JBQXVCO0VBQ3ZCLGlCQUFnQjtFQUNoQixXQUFVLEVBU1g7O0FBZEQ7SUFRSSxjQUFhO0lBQ2Isb0JBQW1CO0lBQ25CLDRCQUEyQjtJQUMzQixrQkFBaUI7SUFDakIsb0JBQW1CLEVBQ3BCOztBQUdIO0VBQ0UsV0FBVSxFQWNYOztBQWZEO0lBSUksY0FBYTtJQUNiLGtCQUFpQjtJQUNqQixvQkFBbUI7SUFDbkIsb0JBQW1CLEVBQ3BCOztBQVJIO0lBV0ksb0JBQW1CO0lBQ25CLFlBQVc7SUFDWCxpQkFBZ0IsRUFDakIiLCJmaWxlIjoic3JjL2FwcC9zZXR0aW5ncy9zZXR0aW5ncy5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIi5oZWFkIHtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IGZsZXgtZW5kO1xyXG59XHJcblxyXG4uc2V0dGluZ3MtdGl0bGUge1xyXG4gIGZvbnQtc2l6ZTogMS43cmVtO1xyXG59XHJcblxyXG4udGhlbWUtc2VsZWN0aW9uIHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgYWxpZ24taXRlbXM6IGZsZXgtc3RhcnQ7XHJcbiAgbWFyZ2luOiAyLjRyZW0gMDtcclxuICB3aWR0aDogNTAlO1xyXG5cclxuICAucmFkaW8tYmxvY2sge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGZsZXgtc3RhcnQ7XHJcbiAgICBmb250LXNpemU6IDEuM3JlbTtcclxuICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XHJcbiAgfVxyXG59XHJcblxyXG4ubWFzdGVyLXBhc3N3b3JkIHtcclxuICB3aWR0aDogNTAlO1xyXG5cclxuICAubWFzdGVyLXBhc3N3b3JkLXRpdGxlIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmb250LXNpemU6IDEuNXJlbTtcclxuICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XHJcbiAgICBtYXJnaW4tYm90dG9tOiAxcmVtO1xyXG4gIH1cclxuXHJcbiAgYnV0dG9uIHtcclxuICAgIG1hcmdpbjogMi41cmVtIGF1dG87XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIG1heC13aWR0aDogMTVyZW07XHJcbiAgfVxyXG59XHJcblxyXG4iXX0= */"

/***/ }),

/***/ "./src/app/settings/settings.component.ts":
/*!************************************************!*\
  !*** ./src/app/settings/settings.component.ts ***!
  \************************************************/
/*! exports provided: SettingsComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "SettingsComponent", function() { return SettingsComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};





var SettingsComponent = /** @class */ (function () {
    function SettingsComponent(renderer, variablesService, backend, location) {
        var _this = this;
        this.renderer = renderer;
        this.variablesService = variablesService;
        this.backend = backend;
        this.location = location;
        this.theme = this.variablesService.settings.theme;
        this.changeForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_3__["FormGroup"]({
            password: new _angular_forms__WEBPACK_IMPORTED_MODULE_3__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_3__["Validators"].required),
            new_password: new _angular_forms__WEBPACK_IMPORTED_MODULE_3__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_3__["Validators"].required),
            new_confirmation: new _angular_forms__WEBPACK_IMPORTED_MODULE_3__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_3__["Validators"].required)
        }, [function (g) {
                return g.get('new_password').value === g.get('new_confirmation').value ? null : { 'confirm_mismatch': true };
            }, function (g) {
                return g.get('password').value === _this.variablesService.appPass ? null : { 'pass_mismatch': true };
            }]);
    }
    SettingsComponent.prototype.ngOnInit = function () { };
    SettingsComponent.prototype.setTheme = function (theme) {
        this.renderer.removeClass(document.body, 'theme-' + this.theme);
        this.theme = theme;
        this.variablesService.settings.theme = this.theme;
        this.renderer.addClass(document.body, 'theme-' + this.theme);
        this.backend.storeAppData();
    };
    SettingsComponent.prototype.onSubmitChangePass = function () {
        var _this = this;
        if (this.changeForm.valid) {
            this.variablesService.appPass = this.changeForm.get('new_password').value;
            this.backend.storeSecureAppData(function (status, data) {
                if (status) {
                    _this.changeForm.reset();
                }
                else {
                    console.log(data);
                }
            });
        }
    };
    SettingsComponent.prototype.back = function () {
        this.location.back();
    };
    SettingsComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-settings',
            template: __webpack_require__(/*! ./settings.component.html */ "./src/app/settings/settings.component.html"),
            styles: [__webpack_require__(/*! ./settings.component.scss */ "./src/app/settings/settings.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_core__WEBPACK_IMPORTED_MODULE_0__["Renderer2"], _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"], _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"], _angular_common__WEBPACK_IMPORTED_MODULE_4__["Location"]])
    ], SettingsComponent);
    return SettingsComponent;
}());



/***/ }),

/***/ "./src/app/sidebar/sidebar.component.html":
/*!************************************************!*\
  !*** ./src/app/sidebar/sidebar.component.html ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"sidebar-accounts\">\r\n  <div class=\"sidebar-accounts-header\">\r\n    <h3>{{ 'SIDEBAR.TITLE' | translate }}</h3><button [routerLink]=\"['main']\">{{ 'SIDEBAR.ADD_NEW' | translate }}</button>\r\n  </div>\r\n  <div class=\"sidebar-accounts-list scrolled-content\">\r\n    <div class=\"sidebar-account\" *ngFor=\"let wallet of variablesService.wallets\" [class.active]=\"wallet?.wallet_id === walletActive\" [routerLink]=\"['/wallet/' + wallet.wallet_id + '/' + walletActiveSubDirectory]\">\r\n      <div class=\"sidebar-account-row account-title-balance\">\r\n        <span class=\"title\">{{wallet.name}}</span>\r\n        <span class=\"balance\">{{wallet.unlocked_balance | intToMoney}} {{variablesService.defaultCurrency}}</span>\r\n      </div>\r\n      <div class=\"sidebar-account-row account-alias\">\r\n        <span>{{wallet.alias}}</span>\r\n        <span>$ {{wallet.getMoneyEquivalent(variablesService.moneyEquivalent) | intToMoney | number : '1.2-2'}}</span>\r\n      </div>\r\n      <div class=\"sidebar-account-row account-staking\" *ngIf=\"!(!wallet.loaded && variablesService.daemon_state === 2)\">\r\n        <span class=\"text\">{{ 'SIDEBAR.ACCOUNT.STAKING' | translate }}</span>\r\n        <app-staking-switch [(wallet_id)]=\"wallet.wallet_id\" [(staking)]=\"wallet.staking\"></app-staking-switch>\r\n      </div>\r\n      <div class=\"sidebar-account-row account-messages\" *ngIf=\"!(!wallet.loaded && variablesService.daemon_state === 2)\">\r\n        <span class=\"text\">{{ 'SIDEBAR.ACCOUNT.MESSAGES' | translate }}</span>\r\n        <span class=\"indicator\">{{wallet.new_contracts}}</span>\r\n      </div>\r\n      <div class=\"sidebar-account-row account-synchronization\" *ngIf=\"!wallet.loaded && variablesService.daemon_state === 2\">\r\n        <span class=\"status\">{{ 'SIDEBAR.SYNCHRONIZATION.SYNCING' | translate }}</span>\r\n        <div class=\"progress-bar-container\">\r\n          <div class=\"progress-bar\">\r\n            <div class=\"fill\" [style.width]=\"wallet.progress + '%'\"></div>\r\n          </div>\r\n          <div class=\"progress-percent\">{{ wallet.progress }}%</div>\r\n        </div>\r\n      </div>\r\n    </div>\r\n  </div>\r\n</div>\r\n<div class=\"sidebar-settings\">\r\n  <button [routerLink]=\"['/settings']\" routerLinkActive=\"active\">\r\n    <i class=\"icon settings\"></i>\r\n    <span>{{ 'SIDEBAR.SETTINGS' | translate }}</span>\r\n  </button>\r\n  <button (click)=\"logOut()\">\r\n    <i class=\"icon logout\"></i>\r\n    <span>{{ 'SIDEBAR.LOG_OUT' | translate }}</span>\r\n  </button>\r\n</div>\r\n<div class=\"sidebar-synchronization-status\">\r\n  <div class=\"status-container\">\r\n    <span class=\"offline\" *ngIf=\"variablesService.daemon_state === 0\">{{ 'SIDEBAR.SYNCHRONIZATION.OFFLINE' | translate }}</span>\r\n    <span class=\"syncing\" *ngIf=\"variablesService.daemon_state === 1\">{{ 'SIDEBAR.SYNCHRONIZATION.SYNCING' | translate }}</span>\r\n    <span class=\"online\" *ngIf=\"variablesService.daemon_state === 2\">{{ 'SIDEBAR.SYNCHRONIZATION.ONLINE' | translate }}</span>\r\n    <span class=\"loading\" *ngIf=\"variablesService.daemon_state === 3\">{{ 'SIDEBAR.SYNCHRONIZATION.LOADING' | translate }}</span>\r\n    <span class=\"offline\" *ngIf=\"variablesService.daemon_state === 4\">{{ 'SIDEBAR.SYNCHRONIZATION.ERROR' | translate }}</span>\r\n    <span class=\"online\" *ngIf=\"variablesService.daemon_state === 5\">{{ 'SIDEBAR.SYNCHRONIZATION.COMPLETE' | translate }}</span>\r\n  </div>\r\n  <div class=\"progress-bar-container\">\r\n    <div class=\"syncing\" *ngIf=\"variablesService.daemon_state === 1\">\r\n      <div class=\"progress-bar\">\r\n        <div class=\"fill\" [style.width]=\"variablesService.sync.progress_value + '%'\"></div>\r\n      </div>\r\n      <div class=\"progress-percent\">{{ variablesService.sync.progress_value_text }}%</div>\r\n    </div>\r\n    <div class=\"loading\" *ngIf=\"variablesService.daemon_state === 3\"></div>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/sidebar/sidebar.component.scss":
/*!************************************************!*\
  !*** ./src/app/sidebar/sidebar.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: flex;\n  flex-direction: column;\n  justify-content: space-between;\n  flex: 0 0 25rem;\n  padding: 0 3rem 3rem; }\n\n.sidebar-accounts {\n  display: flex;\n  flex-direction: column;\n  flex: 1 1 auto; }\n\n.sidebar-accounts .sidebar-accounts-header {\n    display: flex;\n    align-items: center;\n    justify-content: space-between;\n    flex: 0 0 auto;\n    height: 8rem;\n    font-weight: 400; }\n\n.sidebar-accounts .sidebar-accounts-header h3 {\n      font-size: 1.7rem; }\n\n.sidebar-accounts .sidebar-accounts-header button {\n      background: transparent;\n      border: none;\n      outline: none; }\n\n.sidebar-accounts .sidebar-accounts-list {\n    display: flex;\n    flex-direction: column;\n    flex: 1 1 auto;\n    margin: 0 -3rem;\n    overflow-y: overlay; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account {\n      display: flex;\n      flex-direction: column;\n      flex-shrink: 0;\n      cursor: pointer;\n      padding: 2rem 3rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row {\n        display: flex;\n        align-items: center;\n        justify-content: space-between; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-title-balance {\n          line-height: 2.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-title-balance .title {\n            font-size: 1.5rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-title-balance .balance {\n            font-size: 1.8rem;\n            font-weight: 600; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias {\n          font-size: 1.3rem;\n          line-height: 3.4rem;\n          margin-bottom: 0.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-staking {\n          line-height: 2.9rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-staking .text {\n            font-size: 1.3rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-messages {\n          line-height: 2.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-messages .text {\n            font-size: 1.3rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-messages .indicator {\n            display: flex;\n            align-items: center;\n            justify-content: center;\n            border-radius: 1rem;\n            font-size: 1rem;\n            min-width: 24px;\n            height: 16px;\n            padding: 0 5px; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization {\n          flex-direction: column;\n          height: 5.6rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .status {\n            align-self: flex-start;\n            font-size: 1.3rem;\n            line-height: 2.6rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container {\n            display: flex;\n            margin: 0.4rem 0;\n            height: 0.7rem;\n            width: 100%; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container .progress-bar {\n              flex: 1 0 auto; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container .progress-bar .fill {\n                height: 100%; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container .progress-percent {\n              flex: 0 0 auto;\n              font-size: 1.3rem;\n              line-height: 0.7rem;\n              padding-left: 0.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account:focus {\n        outline: none; }\n\n.sidebar-settings {\n  flex: 0 0 auto;\n  padding-bottom: 1rem; }\n\n.sidebar-settings button {\n    display: flex;\n    align-items: center;\n    background: transparent;\n    border: none;\n    line-height: 3rem;\n    outline: none;\n    padding: 0;\n    font-weight: 400; }\n\n.sidebar-settings button .icon {\n      margin-right: 1.2rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.sidebar-settings button .icon.settings {\n        -webkit-mask: url('settings.svg') no-repeat center;\n                mask: url('settings.svg') no-repeat center; }\n\n.sidebar-settings button .icon.logout {\n        -webkit-mask: url('logout.svg') no-repeat center;\n                mask: url('logout.svg') no-repeat center; }\n\n.sidebar-synchronization-status {\n  position: relative;\n  display: flex;\n  align-items: flex-end;\n  justify-content: flex-start;\n  flex: 0 0 4rem;\n  font-size: 1.3rem; }\n\n.sidebar-synchronization-status .status-container .offline, .sidebar-synchronization-status .status-container .online {\n    position: relative;\n    display: block;\n    line-height: 1.2rem;\n    padding-left: 2.2rem; }\n\n.sidebar-synchronization-status .status-container .offline:before, .sidebar-synchronization-status .status-container .online:before {\n      content: '';\n      position: absolute;\n      top: 0;\n      left: 0;\n      border-radius: 50%;\n      width: 1.2rem;\n      height: 1.2rem; }\n\n.sidebar-synchronization-status .status-container .syncing, .sidebar-synchronization-status .status-container .loading {\n    line-height: 4rem; }\n\n.sidebar-synchronization-status .progress-bar-container {\n    position: absolute;\n    bottom: -0.7rem;\n    left: 0;\n    height: 0.7rem;\n    width: 100%; }\n\n.sidebar-synchronization-status .progress-bar-container .syncing {\n      display: flex; }\n\n.sidebar-synchronization-status .progress-bar-container .syncing .progress-bar {\n        flex: 1 0 auto; }\n\n.sidebar-synchronization-status .progress-bar-container .syncing .progress-bar .fill {\n          height: 100%; }\n\n.sidebar-synchronization-status .progress-bar-container .syncing .progress-percent {\n        flex: 0 0 auto;\n        font-size: 1.3rem;\n        line-height: 0.7rem;\n        padding-left: 0.7rem; }\n\n.sidebar-synchronization-status .progress-bar-container .loading {\n      background-image: url('loading.png');\n      height: 100%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvc2lkZWJhci9EOlxcemFuby9zcmNcXGFwcFxcc2lkZWJhclxcc2lkZWJhci5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGNBQWE7RUFDYix1QkFBc0I7RUFDdEIsK0JBQThCO0VBQzlCLGdCQUFlO0VBQ2YscUJBQW9CLEVBQ3JCOztBQUVEO0VBQ0UsY0FBYTtFQUNiLHVCQUFzQjtFQUN0QixlQUFjLEVBNkhmOztBQWhJRDtJQU1JLGNBQWE7SUFDYixvQkFBbUI7SUFDbkIsK0JBQThCO0lBQzlCLGVBQWM7SUFDZCxhQUFZO0lBQ1osaUJBQWdCLEVBV2pCOztBQXRCSDtNQWNNLGtCQUFpQixFQUNsQjs7QUFmTDtNQWtCTSx3QkFBdUI7TUFDdkIsYUFBWTtNQUNaLGNBQWEsRUFDZDs7QUFyQkw7SUF5QkksY0FBYTtJQUNiLHVCQUFzQjtJQUN0QixlQUFjO0lBQ2QsZ0JBQWU7SUFDZixvQkFBbUIsRUFrR3BCOztBQS9ISDtNQWdDTSxjQUFhO01BQ2IsdUJBQXNCO01BQ3RCLGVBQWM7TUFDZCxnQkFBZTtNQUNmLG1CQUFrQixFQTBGbkI7O0FBOUhMO1FBdUNRLGNBQWE7UUFDYixvQkFBbUI7UUFDbkIsK0JBQThCLEVBZ0YvQjs7QUF6SFA7VUE0Q1Usb0JBQW1CLEVBVXBCOztBQXREVDtZQStDWSxrQkFBaUIsRUFDbEI7O0FBaERYO1lBbURZLGtCQUFpQjtZQUNqQixpQkFBZ0IsRUFDakI7O0FBckRYO1VBeURVLGtCQUFpQjtVQUNqQixvQkFBbUI7VUFDbkIsc0JBQXFCLEVBQ3RCOztBQTVEVDtVQStEVSxvQkFBbUIsRUFLcEI7O0FBcEVUO1lBa0VZLGtCQUFpQixFQUNsQjs7QUFuRVg7VUF1RVUsb0JBQW1CLEVBZ0JwQjs7QUF2RlQ7WUEwRVksa0JBQWlCLEVBQ2xCOztBQTNFWDtZQThFWSxjQUFhO1lBQ2Isb0JBQW1CO1lBQ25CLHdCQUF1QjtZQUN2QixvQkFBbUI7WUFDbkIsZ0JBQWU7WUFDZixnQkFBZTtZQUNmLGFBQVk7WUFDWixlQUFjLEVBQ2Y7O0FBdEZYO1VBMEZVLHVCQUFzQjtVQUN0QixlQUFjLEVBNkJmOztBQXhIVDtZQThGWSx1QkFBc0I7WUFDdEIsa0JBQWlCO1lBQ2pCLG9CQUFtQixFQUNwQjs7QUFqR1g7WUFvR1ksY0FBYTtZQUNiLGlCQUFnQjtZQUNoQixlQUFjO1lBQ2QsWUFBVyxFQWdCWjs7QUF2SFg7Y0EwR2MsZUFBYyxFQUtmOztBQS9HYjtnQkE2R2dCLGFBQVksRUFDYjs7QUE5R2Y7Y0FrSGMsZUFBYztjQUNkLGtCQUFpQjtjQUNqQixvQkFBbUI7Y0FDbkIscUJBQW9CLEVBQ3JCOztBQXRIYjtRQTRIUSxjQUFhLEVBQ2Q7O0FBS1A7RUFDRSxlQUFjO0VBQ2QscUJBQW9CLEVBMEJyQjs7QUE1QkQ7SUFLSSxjQUFhO0lBQ2Isb0JBQW1CO0lBQ25CLHdCQUF1QjtJQUN2QixhQUFZO0lBQ1osa0JBQWlCO0lBQ2pCLGNBQWE7SUFDYixXQUFVO0lBQ1YsaUJBQWdCLEVBZWpCOztBQTNCSDtNQWVNLHFCQUFvQjtNQUNwQixjQUFhO01BQ2IsZUFBYyxFQVNmOztBQTFCTDtRQW9CUSxtREFBMkQ7Z0JBQTNELDJDQUEyRCxFQUM1RDs7QUFyQlA7UUF3QlEsaURBQXlEO2dCQUF6RCx5Q0FBeUQsRUFDMUQ7O0FBS1A7RUFDRSxtQkFBa0I7RUFDbEIsY0FBYTtFQUNiLHNCQUFxQjtFQUNyQiw0QkFBMkI7RUFDM0IsZUFBYztFQUNkLGtCQUFpQixFQXlEbEI7O0FBL0REO0lBV00sbUJBQWtCO0lBQ2xCLGVBQWM7SUFDZCxvQkFBbUI7SUFDbkIscUJBQW9CLEVBV3JCOztBQXpCTDtNQWlCUSxZQUFXO01BQ1gsbUJBQWtCO01BQ2xCLE9BQU07TUFDTixRQUFPO01BQ1AsbUJBQWtCO01BQ2xCLGNBQWE7TUFDYixlQUFjLEVBQ2Y7O0FBeEJQO0lBNEJNLGtCQUFpQixFQUNsQjs7QUE3Qkw7SUFpQ0ksbUJBQWtCO0lBQ2xCLGdCQUFlO0lBQ2YsUUFBTztJQUNQLGVBQWM7SUFDZCxZQUFXLEVBeUJaOztBQTlESDtNQXdDTSxjQUFhLEVBZ0JkOztBQXhETDtRQTJDUSxlQUFjLEVBS2Y7O0FBaERQO1VBOENVLGFBQVksRUFDYjs7QUEvQ1Q7UUFtRFEsZUFBYztRQUNkLGtCQUFpQjtRQUNqQixvQkFBbUI7UUFDbkIscUJBQW9CLEVBQ3JCOztBQXZEUDtNQTJETSxxQ0FBd0Q7TUFDeEQsYUFBWSxFQUNiIiwiZmlsZSI6InNyYy9hcHAvc2lkZWJhci9zaWRlYmFyLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgZmxleDogMCAwIDI1cmVtO1xyXG4gIHBhZGRpbmc6IDAgM3JlbSAzcmVtO1xyXG59XHJcblxyXG4uc2lkZWJhci1hY2NvdW50cyB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gIGZsZXg6IDEgMSBhdXRvO1xyXG5cclxuICAuc2lkZWJhci1hY2NvdW50cy1oZWFkZXIge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgICBmbGV4OiAwIDAgYXV0bztcclxuICAgIGhlaWdodDogOHJlbTtcclxuICAgIGZvbnQtd2VpZ2h0OiA0MDA7XHJcblxyXG4gICAgaDMge1xyXG4gICAgICBmb250LXNpemU6IDEuN3JlbTtcclxuICAgIH1cclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBiYWNrZ3JvdW5kOiB0cmFuc3BhcmVudDtcclxuICAgICAgYm9yZGVyOiBub25lO1xyXG4gICAgICBvdXRsaW5lOiBub25lO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnNpZGViYXItYWNjb3VudHMtbGlzdCB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgIGZsZXg6IDEgMSBhdXRvO1xyXG4gICAgbWFyZ2luOiAwIC0zcmVtO1xyXG4gICAgb3ZlcmZsb3cteTogb3ZlcmxheTtcclxuXHJcbiAgICAuc2lkZWJhci1hY2NvdW50IHtcclxuICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgICAgZmxleC1zaHJpbms6IDA7XHJcbiAgICAgIGN1cnNvcjogcG9pbnRlcjtcclxuICAgICAgcGFkZGluZzogMnJlbSAzcmVtO1xyXG5cclxuICAgICAgLnNpZGViYXItYWNjb3VudC1yb3cge1xyXG4gICAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgICAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcblxyXG4gICAgICAgICYuYWNjb3VudC10aXRsZS1iYWxhbmNlIHtcclxuICAgICAgICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XHJcblxyXG4gICAgICAgICAgLnRpdGxlIHtcclxuICAgICAgICAgICAgZm9udC1zaXplOiAxLjVyZW07XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgLmJhbGFuY2Uge1xyXG4gICAgICAgICAgICBmb250LXNpemU6IDEuOHJlbTtcclxuICAgICAgICAgICAgZm9udC13ZWlnaHQ6IDYwMDtcclxuICAgICAgICAgIH1cclxuICAgICAgICB9XHJcblxyXG4gICAgICAgICYuYWNjb3VudC1hbGlhcyB7XHJcbiAgICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcclxuICAgICAgICAgIGxpbmUtaGVpZ2h0OiAzLjRyZW07XHJcbiAgICAgICAgICBtYXJnaW4tYm90dG9tOiAwLjdyZW07XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAmLmFjY291bnQtc3Rha2luZyB7XHJcbiAgICAgICAgICBsaW5lLWhlaWdodDogMi45cmVtO1xyXG5cclxuICAgICAgICAgIC50ZXh0IHtcclxuICAgICAgICAgICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICAgICAgICB9XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAmLmFjY291bnQtbWVzc2FnZXMge1xyXG4gICAgICAgICAgbGluZS1oZWlnaHQ6IDIuN3JlbTtcclxuXHJcbiAgICAgICAgICAudGV4dCB7XHJcbiAgICAgICAgICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgICAgICAgfVxyXG5cclxuICAgICAgICAgIC5pbmRpY2F0b3Ige1xyXG4gICAgICAgICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICAgICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICAgICAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgICAgICAgICAgYm9yZGVyLXJhZGl1czogMXJlbTtcclxuICAgICAgICAgICAgZm9udC1zaXplOiAxcmVtO1xyXG4gICAgICAgICAgICBtaW4td2lkdGg6IDI0cHg7XHJcbiAgICAgICAgICAgIGhlaWdodDogMTZweDtcclxuICAgICAgICAgICAgcGFkZGluZzogMCA1cHg7XHJcbiAgICAgICAgICB9XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAmLmFjY291bnQtc3luY2hyb25pemF0aW9uIHtcclxuICAgICAgICAgIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgICAgICAgICBoZWlnaHQ6IDUuNnJlbTtcclxuXHJcbiAgICAgICAgICAuc3RhdHVzIHtcclxuICAgICAgICAgICAgYWxpZ24tc2VsZjogZmxleC1zdGFydDtcclxuICAgICAgICAgICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICAgICAgICAgIGxpbmUtaGVpZ2h0OiAyLjZyZW07XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgLnByb2dyZXNzLWJhci1jb250YWluZXIge1xyXG4gICAgICAgICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICAgICAgICBtYXJnaW46IDAuNHJlbSAwO1xyXG4gICAgICAgICAgICBoZWlnaHQ6IDAuN3JlbTtcclxuICAgICAgICAgICAgd2lkdGg6IDEwMCU7XHJcblxyXG4gICAgICAgICAgICAucHJvZ3Jlc3MtYmFyIHtcclxuICAgICAgICAgICAgICBmbGV4OiAxIDAgYXV0bztcclxuXHJcbiAgICAgICAgICAgICAgLmZpbGwge1xyXG4gICAgICAgICAgICAgICAgaGVpZ2h0OiAxMDAlO1xyXG4gICAgICAgICAgICAgIH1cclxuICAgICAgICAgICAgfVxyXG5cclxuICAgICAgICAgICAgLnByb2dyZXNzLXBlcmNlbnQge1xyXG4gICAgICAgICAgICAgIGZsZXg6IDAgMCBhdXRvO1xyXG4gICAgICAgICAgICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgICAgICAgICAgIGxpbmUtaGVpZ2h0OiAwLjdyZW07XHJcbiAgICAgICAgICAgICAgcGFkZGluZy1sZWZ0OiAwLjdyZW07XHJcbiAgICAgICAgICAgIH1cclxuICAgICAgICAgIH1cclxuICAgICAgICB9XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICY6Zm9jdXMge1xyXG4gICAgICAgIG91dGxpbmU6IG5vbmU7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcbn1cclxuXHJcbi5zaWRlYmFyLXNldHRpbmdzIHtcclxuICBmbGV4OiAwIDAgYXV0bztcclxuICBwYWRkaW5nLWJvdHRvbTogMXJlbTtcclxuXHJcbiAgYnV0dG9uIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgYmFja2dyb3VuZDogdHJhbnNwYXJlbnQ7XHJcbiAgICBib3JkZXI6IG5vbmU7XHJcbiAgICBsaW5lLWhlaWdodDogM3JlbTtcclxuICAgIG91dGxpbmU6IG5vbmU7XHJcbiAgICBwYWRkaW5nOiAwO1xyXG4gICAgZm9udC13ZWlnaHQ6IDQwMDtcclxuXHJcbiAgICAuaWNvbiB7XHJcbiAgICAgIG1hcmdpbi1yaWdodDogMS4ycmVtO1xyXG4gICAgICB3aWR0aDogMS43cmVtO1xyXG4gICAgICBoZWlnaHQ6IDEuN3JlbTtcclxuXHJcbiAgICAgICYuc2V0dGluZ3Mge1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvc2V0dGluZ3Muc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLmxvZ291dCB7XHJcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9sb2dvdXQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcblxyXG4uc2lkZWJhci1zeW5jaHJvbml6YXRpb24tc3RhdHVzIHtcclxuICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBhbGlnbi1pdGVtczogZmxleC1lbmQ7XHJcbiAganVzdGlmeS1jb250ZW50OiBmbGV4LXN0YXJ0O1xyXG4gIGZsZXg6IDAgMCA0cmVtO1xyXG4gIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG5cclxuICAuc3RhdHVzLWNvbnRhaW5lciB7XHJcblxyXG4gICAgLm9mZmxpbmUsIC5vbmxpbmUge1xyXG4gICAgICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgICAgIGRpc3BsYXk6IGJsb2NrO1xyXG4gICAgICBsaW5lLWhlaWdodDogMS4ycmVtO1xyXG4gICAgICBwYWRkaW5nLWxlZnQ6IDIuMnJlbTtcclxuXHJcbiAgICAgICY6YmVmb3JlIHtcclxuICAgICAgICBjb250ZW50OiAnJztcclxuICAgICAgICBwb3NpdGlvbjogYWJzb2x1dGU7XHJcbiAgICAgICAgdG9wOiAwO1xyXG4gICAgICAgIGxlZnQ6IDA7XHJcbiAgICAgICAgYm9yZGVyLXJhZGl1czogNTAlO1xyXG4gICAgICAgIHdpZHRoOiAxLjJyZW07XHJcbiAgICAgICAgaGVpZ2h0OiAxLjJyZW07XHJcbiAgICAgIH1cclxuICAgIH1cclxuXHJcbiAgICAuc3luY2luZywgLmxvYWRpbmcge1xyXG4gICAgICBsaW5lLWhlaWdodDogNHJlbTtcclxuICAgIH1cclxuICB9XHJcblxyXG4gIC5wcm9ncmVzcy1iYXItY29udGFpbmVyIHtcclxuICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuICAgIGJvdHRvbTogLTAuN3JlbTtcclxuICAgIGxlZnQ6IDA7XHJcbiAgICBoZWlnaHQ6IDAuN3JlbTtcclxuICAgIHdpZHRoOiAxMDAlO1xyXG5cclxuICAgIC5zeW5jaW5nIHtcclxuICAgICAgZGlzcGxheTogZmxleDtcclxuXHJcbiAgICAgIC5wcm9ncmVzcy1iYXIge1xyXG4gICAgICAgIGZsZXg6IDEgMCBhdXRvO1xyXG5cclxuICAgICAgICAuZmlsbCB7XHJcbiAgICAgICAgICBoZWlnaHQ6IDEwMCU7XHJcbiAgICAgICAgfVxyXG4gICAgICB9XHJcblxyXG4gICAgICAucHJvZ3Jlc3MtcGVyY2VudCB7XHJcbiAgICAgICAgZmxleDogMCAwIGF1dG87XHJcbiAgICAgICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICAgICAgbGluZS1oZWlnaHQ6IDAuN3JlbTtcclxuICAgICAgICBwYWRkaW5nLWxlZnQ6IDAuN3JlbTtcclxuICAgICAgfVxyXG4gICAgfVxyXG5cclxuICAgIC5sb2FkaW5nIHtcclxuICAgICAgYmFja2dyb3VuZC1pbWFnZTogdXJsKFwiLi4vLi4vYXNzZXRzL2ltYWdlcy9sb2FkaW5nLnBuZ1wiKTtcclxuICAgICAgaGVpZ2h0OiAxMDAlO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

/***/ }),

/***/ "./src/app/sidebar/sidebar.component.ts":
/*!**********************************************!*\
  !*** ./src/app/sidebar/sidebar.component.ts ***!
  \**********************************************/
/*! exports provided: SidebarComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "SidebarComponent", function() { return SidebarComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};



var SidebarComponent = /** @class */ (function () {
    function SidebarComponent(route, router, variablesService) {
        this.route = route;
        this.router = router;
        this.variablesService = variablesService;
        this.walletActiveSubDirectory = '';
    }
    SidebarComponent.prototype.ngOnInit = function () {
        var _this = this;
        if (this.router.url.indexOf('/wallet/') !== -1) {
            var localPathArr = this.router.url.split('/');
            if (localPathArr.length >= 3) {
                this.walletActive = parseInt(localPathArr[2], 10);
            }
            if (localPathArr.length >= 4) {
                this.walletActiveSubDirectory = localPathArr[3];
            }
        }
        else if (this.router.url.indexOf('/details') !== -1) {
            this.walletActive = this.variablesService.currentWallet.wallet_id;
        }
        else {
            this.walletActive = null;
        }
        this.walletSubRouting = this.router.events.subscribe(function (event) {
            if (event instanceof _angular_router__WEBPACK_IMPORTED_MODULE_1__["NavigationStart"]) {
                if (event.url.indexOf('/wallet/') !== -1) {
                    var localPathArr = event.url.split('/');
                    if (localPathArr.length >= 3) {
                        _this.walletActive = parseInt(localPathArr[2], 10);
                    }
                    if (localPathArr.length >= 4) {
                        _this.walletActiveSubDirectory = localPathArr[3];
                        if (_this.walletActiveSubDirectory === 'purchase') {
                            _this.walletActiveSubDirectory = 'contracts';
                        }
                    }
                }
                else if (event.url.indexOf('/details') !== -1) {
                    _this.walletActive = _this.variablesService.currentWallet.wallet_id;
                }
                else {
                    _this.walletActive = null;
                }
            }
        });
    };
    SidebarComponent.prototype.ngOnDestroy = function () {
        this.walletSubRouting.unsubscribe();
    };
    SidebarComponent.prototype.logOut = function () {
        this.variablesService.stopCountdown();
        this.variablesService.appPass = '';
        this.router.navigate(['/login'], { queryParams: { type: 'auth' } });
    };
    SidebarComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-sidebar',
            template: __webpack_require__(/*! ./sidebar.component.html */ "./src/app/sidebar/sidebar.component.html"),
            styles: [__webpack_require__(/*! ./sidebar.component.scss */ "./src/app/sidebar/sidebar.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_1__["ActivatedRoute"],
            _angular_router__WEBPACK_IMPORTED_MODULE_1__["Router"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"]])
    ], SidebarComponent);
    return SidebarComponent;
}());



/***/ }),

/***/ "./src/app/staking/staking.component.html":
/*!************************************************!*\
  !*** ./src/app/staking/staking.component.html ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"chart-header\">\r\n  <div class=\"general\">\r\n    <div>\r\n      <span class=\"label\">Staking</span>\r\n      <span class=\"value\">\r\n        <app-staking-switch [(wallet_id)]=\"variablesService.currentWallet.wallet_id\" [(staking)]=\"variablesService.currentWallet.staking\"></app-staking-switch>\r\n      </span>\r\n    </div>\r\n    <div>\r\n      <span class=\"label\">Pending</span>\r\n      <span class=\"value\">{{pending.total | intToMoney}} {{variablesService.defaultCurrency}}</span>\r\n    </div>\r\n    <div>\r\n      <span class=\"label\">Total</span>\r\n      <span class=\"value\">{{total | intToMoney}} {{variablesService.defaultCurrency}}</span>\r\n    </div>\r\n  </div>\r\n  <div class=\"selected\" *ngIf=\"selectedDate && selectedDate.date\">\r\n    <span>{{selectedDate.date | date : 'MMM. EEEE, dd, yyyy'}}</span>\r\n    <span>{{selectedDate.amount}} {{variablesService.defaultCurrency}}</span>\r\n  </div>\r\n</div>\r\n\r\n<div class=\"chart\">\r\n\r\n  <div [chart]=\"chart\"></div>\r\n\r\n</div>\r\n\r\n<div class=\"chart-options\">\r\n  <div class=\"title\">\r\n    Time period:\r\n  </div>\r\n  <div class=\"options\">\r\n    <ng-container *ngFor=\"let period of periods\">\r\n      <button type=\"button\" [class.active]=\"period.active\" (click)=\"changePeriod(period)\">{{period.title}}</button>\r\n    </ng-container>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/staking/staking.component.scss":
/*!************************************************!*\
  !*** ./src/app/staking/staking.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: flex;\n  flex-direction: column;\n  width: 100%; }\n\n.chart-header {\n  display: flex;\n  flex: 0 0 auto; }\n\n.chart-header .general {\n    display: flex;\n    flex-direction: column;\n    align-items: flex-start;\n    justify-content: center;\n    flex-grow: 1;\n    font-size: 1.3rem;\n    margin: -0.5rem 0; }\n\n.chart-header .general > div {\n      display: flex;\n      align-items: center;\n      margin: 0.5rem 0;\n      height: 2rem; }\n\n.chart-header .general > div .label {\n        display: inline-block;\n        width: 9rem; }\n\n.chart-header .selected {\n    display: flex;\n    flex-direction: column;\n    align-items: flex-end;\n    justify-content: center;\n    flex-grow: 1;\n    font-size: 1.8rem; }\n\n.chart-header .selected span {\n      line-height: 2.9rem; }\n\n.chart {\n  display: flex;\n  align-items: center;\n  flex: 1 1 auto;\n  min-height: 40rem; }\n\n.chart > div {\n    width: 100%;\n    height: 100%; }\n\n.chart-options {\n  display: flex;\n  align-items: center;\n  height: 2.4rem;\n  flex: 0 0 auto; }\n\n.chart-options .title {\n    font-size: 1.3rem;\n    width: 9rem; }\n\n.chart-options .options {\n    display: flex;\n    justify-content: space-between;\n    flex-grow: 1;\n    height: 100%; }\n\n.chart-options .options button {\n      display: flex;\n      align-items: center;\n      justify-content: center;\n      flex: 1 1 auto;\n      cursor: pointer;\n      font-size: 1.3rem;\n      margin: 0 0.1rem;\n      padding: 0;\n      height: 100%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvc3Rha2luZy9EOlxcemFuby9zcmNcXGFwcFxcc3Rha2luZ1xcc3Rha2luZy5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGNBQWE7RUFDYix1QkFBc0I7RUFDdEIsWUFBVyxFQUNaOztBQUVEO0VBQ0UsY0FBYTtFQUNiLGVBQWMsRUFvQ2Y7O0FBdENEO0lBS0ksY0FBYTtJQUNiLHVCQUFzQjtJQUN0Qix3QkFBdUI7SUFDdkIsd0JBQXVCO0lBQ3ZCLGFBQVk7SUFDWixrQkFBaUI7SUFDakIsa0JBQWlCLEVBYWxCOztBQXhCSDtNQWNNLGNBQWE7TUFDYixvQkFBbUI7TUFDbkIsaUJBQWdCO01BQ2hCLGFBQVksRUFNYjs7QUF2Qkw7UUFvQlEsc0JBQXFCO1FBQ3JCLFlBQVcsRUFDWjs7QUF0QlA7SUEyQkksY0FBYTtJQUNiLHVCQUFzQjtJQUN0QixzQkFBcUI7SUFDckIsd0JBQXVCO0lBQ3ZCLGFBQVk7SUFDWixrQkFBaUIsRUFLbEI7O0FBckNIO01BbUNNLG9CQUFtQixFQUNwQjs7QUFJTDtFQUNFLGNBQWE7RUFDYixvQkFBbUI7RUFDbkIsZUFBYztFQUNkLGtCQUFpQixFQU1sQjs7QUFWRDtJQU9JLFlBQVc7SUFDWCxhQUFZLEVBQ2I7O0FBR0g7RUFDRSxjQUFhO0VBQ2Isb0JBQW1CO0VBQ25CLGVBQWM7RUFDZCxlQUFjLEVBeUJmOztBQTdCRDtJQU9JLGtCQUFpQjtJQUNqQixZQUFXLEVBQ1o7O0FBVEg7SUFZSSxjQUFhO0lBQ2IsK0JBQThCO0lBQzlCLGFBQVk7SUFDWixhQUFZLEVBYWI7O0FBNUJIO01Ba0JNLGNBQWE7TUFDYixvQkFBbUI7TUFDbkIsd0JBQXVCO01BQ3ZCLGVBQWM7TUFDZCxnQkFBZTtNQUNmLGtCQUFpQjtNQUNqQixpQkFBZ0I7TUFDaEIsV0FBVTtNQUNWLGFBQVksRUFDYiIsImZpbGUiOiJzcmMvYXBwL3N0YWtpbmcvc3Rha2luZy5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgd2lkdGg6IDEwMCU7XHJcbn1cclxuXHJcbi5jaGFydC1oZWFkZXIge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleDogMCAwIGF1dG87XHJcblxyXG4gIC5nZW5lcmFsIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gICAgYWxpZ24taXRlbXM6IGZsZXgtc3RhcnQ7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgIGZsZXgtZ3JvdzogMTtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbWFyZ2luOiAtMC41cmVtIDA7XHJcblxyXG4gICAgPiBkaXYge1xyXG4gICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICBtYXJnaW46IDAuNXJlbSAwO1xyXG4gICAgICBoZWlnaHQ6IDJyZW07XHJcblxyXG4gICAgICAubGFiZWwge1xyXG4gICAgICAgIGRpc3BsYXk6IGlubGluZS1ibG9jaztcclxuICAgICAgICB3aWR0aDogOXJlbTtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnNlbGVjdGVkIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gICAgYWxpZ24taXRlbXM6IGZsZXgtZW5kO1xyXG4gICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XHJcbiAgICBmbGV4LWdyb3c6IDE7XHJcbiAgICBmb250LXNpemU6IDEuOHJlbTtcclxuXHJcbiAgICBzcGFuIHtcclxuICAgICAgbGluZS1oZWlnaHQ6IDIuOXJlbTtcclxuICAgIH1cclxuICB9XHJcbn1cclxuXHJcbi5jaGFydCB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gIGZsZXg6IDEgMSBhdXRvO1xyXG4gIG1pbi1oZWlnaHQ6IDQwcmVtO1xyXG5cclxuICA+IGRpdiB7XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIGhlaWdodDogMTAwJTtcclxuICB9XHJcbn1cclxuXHJcbi5jaGFydC1vcHRpb25zIHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgaGVpZ2h0OiAyLjRyZW07XHJcbiAgZmxleDogMCAwIGF1dG87XHJcblxyXG4gIC50aXRsZSB7XHJcbiAgICBmb250LXNpemU6IDEuM3JlbTtcclxuICAgIHdpZHRoOiA5cmVtO1xyXG4gIH1cclxuXHJcbiAgLm9wdGlvbnMge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICAgIGZsZXgtZ3JvdzogMTtcclxuICAgIGhlaWdodDogMTAwJTtcclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgICAgZmxleDogMSAxIGF1dG87XHJcbiAgICAgIGN1cnNvcjogcG9pbnRlcjtcclxuICAgICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICAgIG1hcmdpbjogMCAwLjFyZW07XHJcbiAgICAgIHBhZGRpbmc6IDA7XHJcbiAgICAgIGhlaWdodDogMTAwJTtcclxuICAgIH1cclxuICB9XHJcbn1cclxuIl19 */"

/***/ }),

/***/ "./src/app/staking/staking.component.ts":
/*!**********************************************!*\
  !*** ./src/app/staking/staking.component.ts ***!
  \**********************************************/
/*! exports provided: StakingComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "StakingComponent", function() { return StakingComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var angular_highcharts__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! angular-highcharts */ "./node_modules/angular-highcharts/fesm5/angular-highcharts.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/pipes/int-to-money.pipe */ "./src/app/_helpers/pipes/int-to-money.pipe.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var StakingComponent = /** @class */ (function () {
    function StakingComponent(route, variablesService, backend, ngZone, intToMoneyPipe) {
        this.route = route;
        this.variablesService = variablesService;
        this.backend = backend;
        this.ngZone = ngZone;
        this.intToMoneyPipe = intToMoneyPipe;
        this.periods = [
            {
                title: '1 day',
                active: false
            },
            {
                title: '1 week',
                active: false
            },
            {
                title: '1 month',
                active: false
            },
            {
                title: '1 year',
                active: false
            },
            {
                title: 'All',
                active: true
            }
        ];
        this.selectedDate = {
            date: null,
            amount: null
        };
        this.originalData = [];
        this.total = 0;
        this.pending = {
            list: [],
            total: 0
        };
    }
    StakingComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function () {
            _this.getMiningHistory();
        });
        this.heightAppEvent = this.variablesService.getHeightAppEvent.subscribe(function (newHeight) {
            if (_this.pending.total) {
                var pendingCount = _this.pending.list.length;
                for (var i = pendingCount - 1; i >= 0; i--) {
                    if (newHeight - _this.pending.list[i].h >= 10) {
                        _this.pending.list.splice(i, 1);
                    }
                }
                if (pendingCount !== _this.pending.list.length) {
                    _this.pending.total = 0;
                    for (var i = 0; i < _this.pending.list.length; i++) {
                        _this.pending.total += _this.pending.list[i].a;
                    }
                }
            }
        });
        this.refreshStackingEvent = this.variablesService.getRefreshStackingEvent.subscribe(function (wallet_id) {
            if (_this.variablesService.currentWallet.wallet_id === wallet_id) {
                _this.getMiningHistory();
            }
        });
    };
    StakingComponent.prototype.drawChart = function (data) {
        var _this = this;
        this.chart = new angular_highcharts__WEBPACK_IMPORTED_MODULE_2__["Chart"]({
            title: { text: '' },
            credits: { enabled: false },
            exporting: { enabled: false },
            legend: { enabled: false },
            chart: {
                type: 'line',
                backgroundColor: 'transparent',
                height: null,
                zoomType: null
            },
            yAxis: {
                min: 0,
                tickAmount: 5,
                title: {
                    text: ''
                },
                gridLineColor: '#2b3644',
                gridLineWidth: 2,
                lineColor: '#2b3644',
                lineWidth: 2,
                tickWidth: 2,
                tickLength: 120,
                tickColor: '#2b3644',
                labels: {
                    y: -8,
                    align: 'left',
                    x: -120,
                    style: {
                        'color': '#e0e0e0',
                        'fontSize': '13px'
                    },
                    format: '{value} ' + this.variablesService.defaultCurrency
                },
                showLastLabel: false,
            },
            xAxis: {
                type: 'datetime',
                gridLineColor: '#2b3644',
                lineColor: '#2b3644',
                lineWidth: 2,
                tickWidth: 2,
                tickLength: 10,
                tickColor: '#2b3644',
                labels: {
                    style: {
                        'color': '#e0e0e0',
                        'fontSize': '13px'
                    }
                },
                minPadding: 0,
                maxPadding: 0,
                minRange: 86400000,
                // tickInterval: 86400000,
                minTickInterval: 3600000,
            },
            tooltip: {
                enabled: false
            },
            plotOptions: {
                area: {
                    fillColor: {
                        linearGradient: {
                            x1: 0,
                            y1: 0,
                            x2: 0,
                            y2: 1
                        },
                        stops: [
                            [0, 'rgba(124,181,236,0.2)'],
                            [1, 'rgba(124,181,236,0)']
                        ]
                    },
                    marker: {
                        enabled: false,
                        radius: 2
                    },
                    lineWidth: 2,
                    threshold: null
                },
                series: {
                    point: {
                        events: {
                            mouseOver: function (obj) {
                                _this.selectedDate.date = obj.target['x'];
                                _this.selectedDate.amount = obj.target['y'];
                            }
                        }
                    },
                    events: {
                        mouseOut: function () {
                            _this.selectedDate.date = null;
                            _this.selectedDate.amount = null;
                        }
                    }
                }
            },
            series: [
                {
                    type: 'area',
                    data: data
                }
            ]
        });
    };
    StakingComponent.prototype.getMiningHistory = function () {
        var _this = this;
        if (this.variablesService.currentWallet.loaded) {
            this.backend.getMiningHistory(this.variablesService.currentWallet.wallet_id, function (status, data) {
                _this.total = 0;
                _this.pending.list = [];
                _this.pending.total = 0;
                _this.originalData = [];
                if (data.mined_entries) {
                    data.mined_entries.forEach(function (item, key) {
                        if (item.t.toString().length === 10) {
                            data.mined_entries[key].t = (new Date(item.t * 1000)).setUTCMilliseconds(0);
                        }
                    });
                    data.mined_entries.forEach(function (item) {
                        _this.total += item.a;
                        if (_this.variablesService.height_app - item.h < 10) {
                            _this.pending.list.push(item);
                            _this.pending.total += item.a;
                        }
                        _this.originalData.push([parseInt(item.t, 10), parseFloat(_this.intToMoneyPipe.transform(item.a))]);
                    });
                    _this.originalData = _this.originalData.sort(function (a, b) {
                        return a[0] - b[0];
                    });
                }
                _this.ngZone.run(function () {
                    _this.drawChart(JSON.parse(JSON.stringify(_this.originalData)));
                });
            });
        }
    };
    StakingComponent.prototype.changePeriod = function (period) {
        this.periods.forEach(function (p) {
            p.active = false;
        });
        period.active = true;
        var d = new Date();
        var min = null;
        var newData = [];
        if (period.title === '1 day') {
            this.originalData.forEach(function (item) {
                var time = (new Date(item[0])).setUTCMinutes(0, 0, 0);
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] += item[1];
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate() - 1, 0, 0, 0, 0);
        }
        else if (period.title === '1 week') {
            this.originalData.forEach(function (item) {
                var time = (new Date(item[0])).setUTCHours(0, 0, 0, 0);
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] += item[1];
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate() - 7, 0, 0, 0, 0);
        }
        else if (period.title === '1 month') {
            this.originalData.forEach(function (item) {
                var time = (new Date(item[0])).setUTCHours(0, 0, 0, 0);
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] += item[1];
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear(), d.getMonth() - 1, d.getDate(), 0, 0, 0, 0);
        }
        else if (period.title === '1 year') {
            this.originalData.forEach(function (item) {
                var time = (new Date(item[0])).setUTCHours(0, 0, 0, 0);
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] += item[1];
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear() - 1, d.getMonth(), d.getDate(), 0, 0, 0, 0);
        }
        else {
            this.chart.ref.series[0].setData(this.originalData, true);
        }
        this.chart.ref.xAxis[0].setExtremes(min, null);
    };
    StakingComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
        this.heightAppEvent.unsubscribe();
        this.refreshStackingEvent.unsubscribe();
    };
    StakingComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-staking',
            template: __webpack_require__(/*! ./staking.component.html */ "./src/app/staking/staking.component.html"),
            styles: [__webpack_require__(/*! ./staking.component.scss */ "./src/app/staking/staking.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_4__["ActivatedRoute"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_5__["IntToMoneyPipe"]])
    ], StakingComponent);
    return StakingComponent;
}());



/***/ }),

/***/ "./src/app/typing-message/typing-message.component.html":
/*!**************************************************************!*\
  !*** ./src/app/typing-message/typing-message.component.html ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"head\">\r\n  <div class=\"interlocutor\">\r\n    @bitmain\r\n  </div>\r\n  <a class=\"back-btn\" [routerLink]=\"['/main']\">\r\n    <i class=\"icon back\"></i>\r\n    <span>{{ 'COMMON.BACK' | translate }}</span>\r\n  </a>\r\n</div>\r\n\r\n<div class=\"messages-content\">\r\n  <div class=\"messages-list scrolled-content\">\r\n    <div class=\"date\">10:39</div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"date\">11:44</div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"date\">12:15</div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"date\">13:13</div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n  </div>\r\n  <div class=\"type-message\">\r\n    <div class=\"input-block textarea\">\r\n      <textarea placeholder=\"{{ 'MESSAGES.SEND_PLACEHOLDER' | translate }}\"></textarea>\r\n    </div>\r\n    <button type=\"button\" class=\"blue-button\">{{ 'MESSAGES.SEND_BUTTON' | translate }}</button>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/typing-message/typing-message.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/typing-message/typing-message.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: flex;\n  flex-direction: column;\n  width: 100%; }\n\n.head {\n  flex: 0 0 auto;\n  box-sizing: content-box;\n  margin: -3rem -3rem 0; }\n\n.messages-content {\n  display: flex;\n  flex-direction: column;\n  justify-content: space-between;\n  flex-grow: 1; }\n\n.messages-content .messages-list {\n    display: flex;\n    flex-direction: column;\n    font-size: 1.3rem;\n    margin: 1rem -3rem;\n    padding: 0 3rem;\n    overflow-y: overlay; }\n\n.messages-content .messages-list div {\n      margin: 0.7rem 0; }\n\n.messages-content .messages-list div.date {\n        text-align: center; }\n\n.messages-content .messages-list div.my, .messages-content .messages-list div.buddy {\n        position: relative;\n        padding: 1.8rem;\n        max-width: 60%; }\n\n.messages-content .messages-list div.buddy {\n        align-self: flex-end; }\n\n.messages-content .type-message {\n    display: flex;\n    flex: 0 0 auto;\n    width: 100%;\n    height: 4.2rem; }\n\n.messages-content .type-message .input-block {\n      width: 100%; }\n\n.messages-content .type-message .input-block > textarea {\n        min-height: 4.2rem; }\n\n.messages-content .type-message button {\n      flex: 0 0 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvdHlwaW5nLW1lc3NhZ2UvRDpcXHphbm8vc3JjXFxhcHBcXHR5cGluZy1tZXNzYWdlXFx0eXBpbmctbWVzc2FnZS5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGNBQWE7RUFDYix1QkFBc0I7RUFDdEIsWUFBVyxFQUNaOztBQUVEO0VBQ0UsZUFBYztFQUNkLHdCQUF1QjtFQUN2QixzQkFBcUIsRUFDdEI7O0FBRUQ7RUFDRSxjQUFhO0VBQ2IsdUJBQXNCO0VBQ3RCLCtCQUE4QjtFQUM5QixhQUFZLEVBK0NiOztBQW5ERDtJQU9JLGNBQWE7SUFDYix1QkFBc0I7SUFDdEIsa0JBQWlCO0lBQ2pCLG1CQUFrQjtJQUNsQixnQkFBZTtJQUNmLG9CQUFtQixFQW1CcEI7O0FBL0JIO01BZU0saUJBQWdCLEVBZWpCOztBQTlCTDtRQWtCUSxtQkFBa0IsRUFDbkI7O0FBbkJQO1FBc0JRLG1CQUFrQjtRQUNsQixnQkFBZTtRQUNmLGVBQWMsRUFDZjs7QUF6QlA7UUE0QlEscUJBQW9CLEVBQ3JCOztBQTdCUDtJQWtDSSxjQUFhO0lBQ2IsZUFBYztJQUNkLFlBQVc7SUFDWCxlQUFjLEVBYWY7O0FBbERIO01Bd0NNLFlBQVcsRUFLWjs7QUE3Q0w7UUEyQ1EsbUJBQWtCLEVBQ25COztBQTVDUDtNQWdETSxnQkFBZSxFQUNoQiIsImZpbGUiOiJzcmMvYXBwL3R5cGluZy1tZXNzYWdlL3R5cGluZy1tZXNzYWdlLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICB3aWR0aDogMTAwJTtcclxufVxyXG5cclxuLmhlYWQge1xyXG4gIGZsZXg6IDAgMCBhdXRvO1xyXG4gIGJveC1zaXppbmc6IGNvbnRlbnQtYm94O1xyXG4gIG1hcmdpbjogLTNyZW0gLTNyZW0gMDtcclxufVxyXG5cclxuLm1lc3NhZ2VzLWNvbnRlbnQge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgZmxleC1ncm93OiAxO1xyXG5cclxuICAubWVzc2FnZXMtbGlzdCB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbWFyZ2luOiAxcmVtIC0zcmVtO1xyXG4gICAgcGFkZGluZzogMCAzcmVtO1xyXG4gICAgb3ZlcmZsb3cteTogb3ZlcmxheTtcclxuXHJcbiAgICBkaXYge1xyXG4gICAgICBtYXJnaW46IDAuN3JlbSAwO1xyXG5cclxuICAgICAgJi5kYXRlIHtcclxuICAgICAgICB0ZXh0LWFsaWduOiBjZW50ZXI7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYubXksICYuYnVkZHkge1xyXG4gICAgICAgIHBvc2l0aW9uOiByZWxhdGl2ZTtcclxuICAgICAgICBwYWRkaW5nOiAxLjhyZW07XHJcbiAgICAgICAgbWF4LXdpZHRoOiA2MCU7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYuYnVkZHkge1xyXG4gICAgICAgIGFsaWduLXNlbGY6IGZsZXgtZW5kO1xyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAudHlwZS1tZXNzYWdlIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmbGV4OiAwIDAgYXV0bztcclxuICAgIHdpZHRoOiAxMDAlO1xyXG4gICAgaGVpZ2h0OiA0LjJyZW07XHJcblxyXG4gICAgLmlucHV0LWJsb2NrIHtcclxuICAgICAgd2lkdGg6IDEwMCU7XHJcblxyXG4gICAgICA+IHRleHRhcmVhIHtcclxuICAgICAgICBtaW4taGVpZ2h0OiA0LjJyZW07XHJcbiAgICAgIH1cclxuICAgIH1cclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBmbGV4OiAwIDAgMTVyZW07XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcblxyXG4iXX0= */"

/***/ }),

/***/ "./src/app/typing-message/typing-message.component.ts":
/*!************************************************************!*\
  !*** ./src/app/typing-message/typing-message.component.ts ***!
  \************************************************************/
/*! exports provided: TypingMessageComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "TypingMessageComponent", function() { return TypingMessageComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};


var TypingMessageComponent = /** @class */ (function () {
    function TypingMessageComponent(route) {
        this.route = route;
        this.route.params.subscribe(function (params) { return console.log(params); });
    }
    TypingMessageComponent.prototype.ngOnInit = function () {
    };
    TypingMessageComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-typing-message',
            template: __webpack_require__(/*! ./typing-message.component.html */ "./src/app/typing-message/typing-message.component.html"),
            styles: [__webpack_require__(/*! ./typing-message.component.scss */ "./src/app/typing-message/typing-message.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_1__["ActivatedRoute"]])
    ], TypingMessageComponent);
    return TypingMessageComponent;
}());



/***/ }),

/***/ "./src/app/wallet-details/wallet-details.component.html":
/*!**************************************************************!*\
  !*** ./src/app/wallet-details/wallet-details.component.html ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span>{{variablesService.currentWallet.name}}</span>\r\n      <span>{{ 'BREADCRUMBS.WALLET_DETAILS' | translate }}</span>\r\n    </div>\r\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <form class=\"form-details\" [formGroup]=\"detailsForm\" (ngSubmit)=\"onSubmitEdit()\">\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-name\">{{ 'WALLET_DETAILS.LABEL_NAME' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\">\r\n    </div>\r\n\r\n    <div class=\"error-block\" *ngIf=\"detailsForm.controls['name'].invalid && (detailsForm.controls['name'].dirty || detailsForm.controls['name'].touched)\">\r\n      <div *ngIf=\"detailsForm.controls['name'].errors['required']\">\r\n        Name is required.\r\n      </div>\r\n      <div *ngIf=\"detailsForm.controls['name'].errors['duplicate']\">\r\n        Name is duplicate.\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-location\">{{ 'WALLET_DETAILS.LABEL_FILE_LOCATION' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-location\" formControlName=\"path\" readonly>\r\n    </div>\r\n    <div class=\"input-block textarea\">\r\n      <label for=\"seed-phrase\">{{ 'WALLET_DETAILS.LABEL_SEED_PHRASE' | translate }}</label>\r\n      <div class=\"seed-phrase\" id=\"seed-phrase\">\r\n        <div class=\"seed-phrase-hint\" (click)=\"showSeedPhrase()\" *ngIf=\"!showSeed\">{{ 'WALLET_DETAILS.SEED_PHRASE_HINT' | translate }}</div>\r\n        <div class=\"seed-phrase-content\" *ngIf=\"showSeed\">\r\n          <ng-container *ngFor=\"let word of seedPhrase.split(' '); let index = index\">\r\n            <div class=\"word\">{{(index + 1) + '. ' + word}}</div>\r\n          </ng-container>\r\n        </div>\r\n      </div>\r\n    </div>\r\n    <div class=\"wallet-buttons\">\r\n      <button type=\"submit\" class=\"blue-button\">{{ 'WALLET_DETAILS.BUTTON_SAVE' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"closeWallet()\">{{ 'WALLET_DETAILS.BUTTON_REMOVE' | translate }}</button>\r\n    </div>\r\n  </form>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/wallet-details/wallet-details.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/wallet-details/wallet-details.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-details {\n  margin-top: 1.8rem; }\n  .form-details .input-block:first-child {\n    width: 50%; }\n  .form-details .seed-phrase {\n    display: flex;\n    font-size: 1.4rem;\n    line-height: 1.5rem;\n    padding: 1.4rem;\n    width: 100%;\n    height: 8.8rem; }\n  .form-details .seed-phrase .seed-phrase-hint {\n      display: flex;\n      align-items: center;\n      justify-content: center;\n      cursor: pointer;\n      width: 100%;\n      height: 100%; }\n  .form-details .seed-phrase .seed-phrase-content {\n      display: flex;\n      flex-direction: column;\n      flex-wrap: wrap;\n      width: 100%;\n      height: 100%; }\n  .form-details .wallet-buttons {\n    display: flex;\n    align-items: center;\n    justify-content: space-between; }\n  .form-details .wallet-buttons button {\n      margin: 2.9rem 0;\n      width: 100%;\n      max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvd2FsbGV0LWRldGFpbHMvRDpcXHphbm8vc3JjXFxhcHBcXHdhbGxldC1kZXRhaWxzXFx3YWxsZXQtZGV0YWlscy5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLG1CQUFrQixFQStDbkI7RUFoREQ7SUFNTSxXQUFVLEVBQ1g7RUFQTDtJQVdJLGNBQWE7SUFDYixrQkFBaUI7SUFDakIsb0JBQW1CO0lBQ25CLGdCQUFlO0lBQ2YsWUFBVztJQUNYLGVBQWMsRUFrQmY7RUFsQ0g7TUFtQk0sY0FBYTtNQUNiLG9CQUFtQjtNQUNuQix3QkFBdUI7TUFDdkIsZ0JBQWU7TUFDZixZQUFXO01BQ1gsYUFBWSxFQUNiO0VBekJMO01BNEJNLGNBQWE7TUFDYix1QkFBc0I7TUFDdEIsZ0JBQWU7TUFDZixZQUFXO01BQ1gsYUFBWSxFQUNiO0VBakNMO0lBcUNJLGNBQWE7SUFDYixvQkFBbUI7SUFDbkIsK0JBQThCLEVBTy9CO0VBOUNIO01BMENNLGlCQUFnQjtNQUNoQixZQUFXO01BQ1gsaUJBQWdCLEVBQ2pCIiwiZmlsZSI6InNyYy9hcHAvd2FsbGV0LWRldGFpbHMvd2FsbGV0LWRldGFpbHMuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyIuZm9ybS1kZXRhaWxzIHtcclxuICBtYXJnaW4tdG9wOiAxLjhyZW07XHJcblxyXG4gIC5pbnB1dC1ibG9jayB7XHJcblxyXG4gICAgJjpmaXJzdC1jaGlsZCB7XHJcbiAgICAgIHdpZHRoOiA1MCU7XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAuc2VlZC1waHJhc2Uge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGZvbnQtc2l6ZTogMS40cmVtO1xyXG4gICAgbGluZS1oZWlnaHQ6IDEuNXJlbTtcclxuICAgIHBhZGRpbmc6IDEuNHJlbTtcclxuICAgIHdpZHRoOiAxMDAlO1xyXG4gICAgaGVpZ2h0OiA4LjhyZW07XHJcblxyXG4gICAgLnNlZWQtcGhyYXNlLWhpbnQge1xyXG4gICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgICAgY3Vyc29yOiBwb2ludGVyO1xyXG4gICAgICB3aWR0aDogMTAwJTtcclxuICAgICAgaGVpZ2h0OiAxMDAlO1xyXG4gICAgfVxyXG5cclxuICAgIC5zZWVkLXBocmFzZS1jb250ZW50IHtcclxuICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgICAgZmxleC13cmFwOiB3cmFwO1xyXG4gICAgICB3aWR0aDogMTAwJTtcclxuICAgICAgaGVpZ2h0OiAxMDAlO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLndhbGxldC1idXR0b25zIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAganVzdGlmeS1jb250ZW50OiBzcGFjZS1iZXR3ZWVuO1xyXG5cclxuICAgIGJ1dHRvbiB7XHJcbiAgICAgIG1hcmdpbjogMi45cmVtIDA7XHJcbiAgICAgIHdpZHRoOiAxMDAlO1xyXG4gICAgICBtYXgtd2lkdGg6IDE1cmVtO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbn1cclxuIl19 */"

/***/ }),

/***/ "./src/app/wallet-details/wallet-details.component.ts":
/*!************************************************************!*\
  !*** ./src/app/wallet-details/wallet-details.component.ts ***!
  \************************************************************/
/*! exports provided: WalletDetailsComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "WalletDetailsComponent", function() { return WalletDetailsComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var WalletDetailsComponent = /** @class */ (function () {
    function WalletDetailsComponent(router, backend, variablesService, ngZone, location) {
        var _this = this;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.location = location;
        this.seedPhrase = '';
        this.showSeed = false;
        this.detailsForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            name: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [_angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required, function (g) {
                    for (var i = 0; i < _this.variablesService.wallets.length; i++) {
                        if (g.value === _this.variablesService.wallets[i].name && _this.variablesService.wallets[i].wallet_id !== _this.variablesService.currentWallet.wallet_id) {
                            return { 'duplicate': true };
                        }
                    }
                    return null;
                }]),
            path: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('')
        });
    }
    WalletDetailsComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.showSeed = false;
        this.detailsForm.get('name').setValue(this.variablesService.currentWallet.name);
        this.detailsForm.get('path').setValue(this.variablesService.currentWallet.path);
        this.backend.getSmartSafeInfo(this.variablesService.currentWallet.wallet_id, function (status, data) {
            if (data.hasOwnProperty('restore_key')) {
                _this.ngZone.run(function () {
                    _this.seedPhrase = data['restore_key'].trim();
                });
            }
        });
    };
    WalletDetailsComponent.prototype.showSeedPhrase = function () {
        this.showSeed = true;
    };
    WalletDetailsComponent.prototype.onSubmitEdit = function () {
        if (this.detailsForm.value) {
            this.variablesService.currentWallet.name = this.detailsForm.get('name').value;
            this.router.navigate(['/wallet/' + this.variablesService.currentWallet.wallet_id]);
        }
    };
    WalletDetailsComponent.prototype.closeWallet = function () {
        var _this = this;
        this.backend.closeWallet(this.variablesService.currentWallet.wallet_id, function () {
            for (var i = _this.variablesService.wallets.length - 1; i >= 0; i--) {
                if (_this.variablesService.wallets[i].wallet_id === _this.variablesService.currentWallet.wallet_id) {
                    _this.variablesService.wallets.splice(i, 1);
                }
            }
            _this.backend.storeSecureAppData(function () {
                _this.ngZone.run(function () {
                    if (_this.variablesService.wallets.length) {
                        _this.variablesService.currentWallet = _this.variablesService.wallets[0];
                        _this.router.navigate(['/wallet/' + _this.variablesService.currentWallet.wallet_id]);
                    }
                    else {
                        _this.router.navigate(['/']);
                    }
                });
            });
        });
    };
    WalletDetailsComponent.prototype.back = function () {
        this.location.back();
    };
    WalletDetailsComponent.prototype.ngOnDestroy = function () { };
    WalletDetailsComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-wallet-details',
            template: __webpack_require__(/*! ./wallet-details.component.html */ "./src/app/wallet-details/wallet-details.component.html"),
            styles: [__webpack_require__(/*! ./wallet-details.component.scss */ "./src/app/wallet-details/wallet-details.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_4__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _angular_common__WEBPACK_IMPORTED_MODULE_5__["Location"]])
    ], WalletDetailsComponent);
    return WalletDetailsComponent;
}());



/***/ }),

/***/ "./src/app/wallet/wallet.component.html":
/*!**********************************************!*\
  !*** ./src/app/wallet/wallet.component.html ***!
  \**********************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"header\">\r\n  <div>\r\n    <h3>{{variablesService.currentWallet.name}}</h3>\r\n    <button>\r\n      <i class=\"icon account\"></i>\r\n      <span>{{ 'WALLET.REGISTER_ALIAS' | translate }}</span>\r\n    </button>\r\n  </div>\r\n  <div>\r\n    <button [routerLink]=\"['/details']\" routerLinkActive=\"active\">\r\n      <i class=\"icon details\"></i>\r\n      <span>{{ 'WALLET.DETAILS' | translate }}</span>\r\n    </button>\r\n    <!--<button (click)=\"closeWallet()\">\r\n      <i class=\"icon lock\"></i>\r\n      <span>{{ 'WALLET.LOCK' | translate }}</span>\r\n    </button>-->\r\n  </div>\r\n</div>\r\n<div class=\"address\">\r\n  <span>{{variablesService.currentWallet.address}}</span>\r\n  <i class=\"icon copy\" (click)=\"copyAddress()\"></i>\r\n</div>\r\n<div class=\"balance\">\r\n  <span>{{variablesService.currentWallet.unlocked_balance | intToMoney}} {{variablesService.defaultCurrency}}</span>\r\n  <span>$ {{variablesService.currentWallet.getMoneyEquivalent(variablesService.moneyEquivalent) | intToMoney | number : '1.2-2'}}</span>\r\n</div>\r\n<div class=\"tabs\">\r\n  <div class=\"tabs-header\">\r\n    <ng-container *ngFor=\"let tab of tabs; let index = index\">\r\n      <div class=\"tab\" [class.active]=\"tab.active\" [class.disabled]=\"(tab.link === '/contracts' || tab.link === '/staking') && variablesService.daemon_state !== 2\" (click)=\"changeTab(index)\">\r\n        <i class=\"icon\" [ngClass]=\"tab.icon\"></i>\r\n        <span>{{ tab.title | translate }}</span>\r\n        <span class=\"indicator\" *ngIf=\"tab.indicator\">{{variablesService.currentWallet.new_contracts}}</span>\r\n      </div>\r\n    </ng-container>\r\n  </div>\r\n  <div class=\"tabs-content scrolled-content\">\r\n    <router-outlet></router-outlet>\r\n  </div>\r\n</div>\r\n\r\n"

/***/ }),

/***/ "./src/app/wallet/wallet.component.scss":
/*!**********************************************!*\
  !*** ./src/app/wallet/wallet.component.scss ***!
  \**********************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: relative;\n  display: flex;\n  flex-direction: column;\n  padding: 0 3rem 3rem;\n  min-width: 95rem;\n  width: 100%;\n  height: 100%; }\n\n.header {\n  display: flex;\n  align-items: center;\n  justify-content: space-between;\n  flex: 0 0 auto;\n  height: 8rem; }\n\n.header > div {\n    display: flex;\n    align-items: center; }\n\n.header > div :not(:last-child) {\n      margin-right: 3.2rem; }\n\n.header h3 {\n    font-size: 1.7rem;\n    font-weight: 600; }\n\n.header button {\n    display: flex;\n    align-items: center;\n    background: transparent;\n    border: none;\n    cursor: pointer;\n    font-weight: 400;\n    outline: none;\n    padding: 0; }\n\n.header button .icon {\n      margin-right: 1.2rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.header button .icon.account {\n        -webkit-mask: url('account.svg') no-repeat center;\n                mask: url('account.svg') no-repeat center; }\n\n.header button .icon.details {\n        -webkit-mask: url('details.svg') no-repeat center;\n                mask: url('details.svg') no-repeat center; }\n\n.header button .icon.lock {\n        -webkit-mask: url('lock.svg') no-repeat center;\n                mask: url('lock.svg') no-repeat center; }\n\n.address {\n  display: flex;\n  align-items: center;\n  flex: 0 0 auto;\n  font-size: 1.4rem;\n  line-height: 1.7rem; }\n\n.address .icon {\n    cursor: pointer;\n    margin-left: 1.2rem;\n    width: 1.7rem;\n    height: 1.7rem; }\n\n.address .icon.copy {\n      -webkit-mask: url('copy.svg') no-repeat center;\n              mask: url('copy.svg') no-repeat center; }\n\n.balance {\n  display: flex;\n  align-items: flex-end;\n  justify-content: flex-start;\n  flex: 0 0 auto;\n  margin: 2.6rem 0; }\n\n.balance :first-child {\n    font-size: 3.3rem;\n    font-weight: 600;\n    line-height: 2.4rem;\n    margin-right: 3.5rem; }\n\n.balance :last-child {\n    font-size: 1.8rem;\n    font-weight: 600;\n    line-height: 1.3rem; }\n\n.tabs {\n  display: flex;\n  flex-direction: column;\n  flex: 1 1 auto; }\n\n.tabs .tabs-header {\n    display: flex;\n    justify-content: space-between;\n    flex: 0 0 auto; }\n\n.tabs .tabs-header .tab {\n      display: flex;\n      align-items: center;\n      justify-content: center;\n      flex: 1 0 auto;\n      cursor: pointer;\n      padding: 0 1rem;\n      height: 5rem; }\n\n.tabs .tabs-header .tab .icon {\n        margin-right: 1.3rem;\n        width: 1.7rem;\n        height: 1.7rem; }\n\n.tabs .tabs-header .tab .icon.send {\n          -webkit-mask: url('send.svg') no-repeat center;\n                  mask: url('send.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.receive {\n          -webkit-mask: url('receive.svg') no-repeat center;\n                  mask: url('receive.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.history {\n          -webkit-mask: url('history.svg') no-repeat center;\n                  mask: url('history.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.contracts {\n          -webkit-mask: url('contracts.svg') no-repeat center;\n                  mask: url('contracts.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.messages {\n          -webkit-mask: url('message.svg') no-repeat center;\n                  mask: url('message.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.staking {\n          -webkit-mask: url('staking.svg') no-repeat center;\n                  mask: url('staking.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .indicator {\n        display: flex;\n        align-items: center;\n        justify-content: center;\n        border-radius: 1rem;\n        font-size: 1rem;\n        font-weight: 600;\n        margin-left: 1.3rem;\n        padding: 0 0.5rem;\n        min-width: 1.6rem;\n        height: 1.6rem; }\n\n.tabs .tabs-header .tab.disabled {\n        cursor: not-allowed; }\n\n.tabs .tabs-header .tab:not(:last-child) {\n        margin-right: 0.3rem; }\n\n.tabs .tabs-content {\n    display: flex;\n    padding: 3rem;\n    flex: 1 1 auto;\n    overflow-x: hidden;\n    overflow-y: overlay; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbInNyYy9hcHAvd2FsbGV0L0Q6XFx6YW5vL3NyY1xcYXBwXFx3YWxsZXRcXHdhbGxldC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLG1CQUFrQjtFQUNsQixjQUFhO0VBQ2IsdUJBQXNCO0VBQ3RCLHFCQUFvQjtFQUNwQixpQkFBZ0I7RUFDaEIsWUFBVztFQUNYLGFBQVksRUFDYjs7QUFFRDtFQUNFLGNBQWE7RUFDYixvQkFBbUI7RUFDbkIsK0JBQThCO0VBQzlCLGVBQWM7RUFDZCxhQUFZLEVBNENiOztBQWpERDtJQVFJLGNBQWE7SUFDYixvQkFBbUIsRUFLcEI7O0FBZEg7TUFZTSxxQkFBb0IsRUFDckI7O0FBYkw7SUFpQkksa0JBQWlCO0lBQ2pCLGlCQUFnQixFQUNqQjs7QUFuQkg7SUFzQkksY0FBYTtJQUNiLG9CQUFtQjtJQUNuQix3QkFBdUI7SUFDdkIsYUFBWTtJQUNaLGdCQUFlO0lBQ2YsaUJBQWdCO0lBQ2hCLGNBQWE7SUFDYixXQUFVLEVBbUJYOztBQWhESDtNQWdDTSxxQkFBb0I7TUFDcEIsY0FBYTtNQUNiLGVBQWMsRUFhZjs7QUEvQ0w7UUFxQ1Esa0RBQTBEO2dCQUExRCwwQ0FBMEQsRUFDM0Q7O0FBdENQO1FBeUNRLGtEQUEwRDtnQkFBMUQsMENBQTBELEVBQzNEOztBQTFDUDtRQTZDUSwrQ0FBdUQ7Z0JBQXZELHVDQUF1RCxFQUN4RDs7QUFLUDtFQUNFLGNBQWE7RUFDYixvQkFBbUI7RUFDbkIsZUFBYztFQUNkLGtCQUFpQjtFQUNqQixvQkFBbUIsRUFZcEI7O0FBakJEO0lBUUksZ0JBQWU7SUFDZixvQkFBbUI7SUFDbkIsY0FBYTtJQUNiLGVBQWMsRUFLZjs7QUFoQkg7TUFjTSwrQ0FBdUQ7Y0FBdkQsdUNBQXVELEVBQ3hEOztBQUlMO0VBQ0UsY0FBYTtFQUNiLHNCQUFxQjtFQUNyQiw0QkFBMkI7RUFDM0IsZUFBYztFQUNkLGlCQUFnQixFQWNqQjs7QUFuQkQ7SUFRSSxrQkFBaUI7SUFDakIsaUJBQWdCO0lBQ2hCLG9CQUFtQjtJQUNuQixxQkFBb0IsRUFDckI7O0FBWkg7SUFlSSxrQkFBaUI7SUFDakIsaUJBQWdCO0lBQ2hCLG9CQUFtQixFQUNwQjs7QUFHSDtFQUNFLGNBQWE7RUFDYix1QkFBc0I7RUFDdEIsZUFBYyxFQTRFZjs7QUEvRUQ7SUFNSSxjQUFhO0lBQ2IsK0JBQThCO0lBQzlCLGVBQWMsRUE4RGY7O0FBdEVIO01BV00sY0FBYTtNQUNiLG9CQUFtQjtNQUNuQix3QkFBdUI7TUFDdkIsZUFBYztNQUNkLGdCQUFlO01BQ2YsZ0JBQWU7TUFDZixhQUFZLEVBb0RiOztBQXJFTDtRQW9CUSxxQkFBb0I7UUFDcEIsY0FBYTtRQUNiLGVBQWMsRUF5QmY7O0FBL0NQO1VBeUJVLCtDQUF1RDtrQkFBdkQsdUNBQXVELEVBQ3hEOztBQTFCVDtVQTZCVSxrREFBMEQ7a0JBQTFELDBDQUEwRCxFQUMzRDs7QUE5QlQ7VUFpQ1Usa0RBQTBEO2tCQUExRCwwQ0FBMEQsRUFDM0Q7O0FBbENUO1VBcUNVLG9EQUE0RDtrQkFBNUQsNENBQTRELEVBQzdEOztBQXRDVDtVQXlDVSxrREFBMEQ7a0JBQTFELDBDQUEwRCxFQUMzRDs7QUExQ1Q7VUE2Q1Usa0RBQTBEO2tCQUExRCwwQ0FBMEQsRUFDM0Q7O0FBOUNUO1FBa0RRLGNBQWE7UUFDYixvQkFBbUI7UUFDbkIsd0JBQXVCO1FBQ3ZCLG9CQUFtQjtRQUNuQixnQkFBZTtRQUNmLGlCQUFnQjtRQUNoQixvQkFBbUI7UUFDbkIsa0JBQWlCO1FBQ2pCLGtCQUFpQjtRQUNqQixlQUFjLEVBQ2Y7O0FBNURQO1FBK0RRLG9CQUFtQixFQUNwQjs7QUFoRVA7UUFtRVEscUJBQW9CLEVBQ3JCOztBQXBFUDtJQXlFSSxjQUFhO0lBQ2IsY0FBYTtJQUNiLGVBQWM7SUFDZCxtQkFBa0I7SUFDbEIsb0JBQW1CLEVBQ3BCIiwiZmlsZSI6InNyYy9hcHAvd2FsbGV0L3dhbGxldC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gIHBhZGRpbmc6IDAgM3JlbSAzcmVtO1xyXG4gIG1pbi13aWR0aDogOTVyZW07XHJcbiAgd2lkdGg6IDEwMCU7XHJcbiAgaGVpZ2h0OiAxMDAlO1xyXG59XHJcblxyXG4uaGVhZGVyIHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAganVzdGlmeS1jb250ZW50OiBzcGFjZS1iZXR3ZWVuO1xyXG4gIGZsZXg6IDAgMCBhdXRvO1xyXG4gIGhlaWdodDogOHJlbTtcclxuXHJcbiAgPiBkaXYge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcblxyXG4gICAgOm5vdCg6bGFzdC1jaGlsZCkge1xyXG4gICAgICBtYXJnaW4tcmlnaHQ6IDMuMnJlbTtcclxuICAgIH1cclxuICB9XHJcblxyXG4gIGgzIHtcclxuICAgIGZvbnQtc2l6ZTogMS43cmVtO1xyXG4gICAgZm9udC13ZWlnaHQ6IDYwMDtcclxuICB9XHJcblxyXG4gIGJ1dHRvbiB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGJhY2tncm91bmQ6IHRyYW5zcGFyZW50O1xyXG4gICAgYm9yZGVyOiBub25lO1xyXG4gICAgY3Vyc29yOiBwb2ludGVyO1xyXG4gICAgZm9udC13ZWlnaHQ6IDQwMDtcclxuICAgIG91dGxpbmU6IG5vbmU7XHJcbiAgICBwYWRkaW5nOiAwO1xyXG5cclxuICAgIC5pY29uIHtcclxuICAgICAgbWFyZ2luLXJpZ2h0OiAxLjJyZW07XHJcbiAgICAgIHdpZHRoOiAxLjdyZW07XHJcbiAgICAgIGhlaWdodDogMS43cmVtO1xyXG5cclxuICAgICAgJi5hY2NvdW50IHtcclxuICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2FjY291bnQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLmRldGFpbHMge1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvZGV0YWlscy5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYubG9jayB7XHJcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9sb2NrLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxufVxyXG5cclxuLmFkZHJlc3Mge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICBmbGV4OiAwIDAgYXV0bztcclxuICBmb250LXNpemU6IDEuNHJlbTtcclxuICBsaW5lLWhlaWdodDogMS43cmVtO1xyXG5cclxuICAuaWNvbiB7XHJcbiAgICBjdXJzb3I6IHBvaW50ZXI7XHJcbiAgICBtYXJnaW4tbGVmdDogMS4ycmVtO1xyXG4gICAgd2lkdGg6IDEuN3JlbTtcclxuICAgIGhlaWdodDogMS43cmVtO1xyXG5cclxuICAgICYuY29weSB7XHJcbiAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvY29weS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcblxyXG4uYmFsYW5jZSB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBhbGlnbi1pdGVtczogZmxleC1lbmQ7XHJcbiAganVzdGlmeS1jb250ZW50OiBmbGV4LXN0YXJ0O1xyXG4gIGZsZXg6IDAgMCBhdXRvO1xyXG4gIG1hcmdpbjogMi42cmVtIDA7XHJcblxyXG4gIDpmaXJzdC1jaGlsZCB7XHJcbiAgICBmb250LXNpemU6IDMuM3JlbTtcclxuICAgIGZvbnQtd2VpZ2h0OiA2MDA7XHJcbiAgICBsaW5lLWhlaWdodDogMi40cmVtO1xyXG4gICAgbWFyZ2luLXJpZ2h0OiAzLjVyZW07XHJcbiAgfVxyXG5cclxuICA6bGFzdC1jaGlsZCB7XHJcbiAgICBmb250LXNpemU6IDEuOHJlbTtcclxuICAgIGZvbnQtd2VpZ2h0OiA2MDA7XHJcbiAgICBsaW5lLWhlaWdodDogMS4zcmVtO1xyXG4gIH1cclxufVxyXG5cclxuLnRhYnMge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICBmbGV4OiAxIDEgYXV0bztcclxuXHJcbiAgLnRhYnMtaGVhZGVyIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgICBmbGV4OiAwIDAgYXV0bztcclxuXHJcbiAgICAudGFiIHtcclxuICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XHJcbiAgICAgIGZsZXg6IDEgMCBhdXRvO1xyXG4gICAgICBjdXJzb3I6IHBvaW50ZXI7XHJcbiAgICAgIHBhZGRpbmc6IDAgMXJlbTtcclxuICAgICAgaGVpZ2h0OiA1cmVtO1xyXG5cclxuICAgICAgLmljb24ge1xyXG4gICAgICAgIG1hcmdpbi1yaWdodDogMS4zcmVtO1xyXG4gICAgICAgIHdpZHRoOiAxLjdyZW07XHJcbiAgICAgICAgaGVpZ2h0OiAxLjdyZW07XHJcblxyXG4gICAgICAgICYuc2VuZCB7XHJcbiAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3NlbmQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgIH1cclxuXHJcbiAgICAgICAgJi5yZWNlaXZlIHtcclxuICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvcmVjZWl2ZS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAmLmhpc3Rvcnkge1xyXG4gICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9oaXN0b3J5LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICB9XHJcblxyXG4gICAgICAgICYuY29udHJhY3RzIHtcclxuICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvY29udHJhY3RzLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICB9XHJcblxyXG4gICAgICAgICYubWVzc2FnZXMge1xyXG4gICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9tZXNzYWdlLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICB9XHJcblxyXG4gICAgICAgICYuc3Rha2luZyB7XHJcbiAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3N0YWtpbmcuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgIH1cclxuICAgICAgfVxyXG5cclxuICAgICAgLmluZGljYXRvciB7XHJcbiAgICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xyXG4gICAgICAgIGJvcmRlci1yYWRpdXM6IDFyZW07XHJcbiAgICAgICAgZm9udC1zaXplOiAxcmVtO1xyXG4gICAgICAgIGZvbnQtd2VpZ2h0OiA2MDA7XHJcbiAgICAgICAgbWFyZ2luLWxlZnQ6IDEuM3JlbTtcclxuICAgICAgICBwYWRkaW5nOiAwIDAuNXJlbTtcclxuICAgICAgICBtaW4td2lkdGg6IDEuNnJlbTtcclxuICAgICAgICBoZWlnaHQ6IDEuNnJlbTtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi5kaXNhYmxlZCB7XHJcbiAgICAgICAgY3Vyc29yOiBub3QtYWxsb3dlZDtcclxuICAgICAgfVxyXG5cclxuICAgICAgJjpub3QoOmxhc3QtY2hpbGQpIHtcclxuICAgICAgICBtYXJnaW4tcmlnaHQ6IDAuM3JlbTtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnRhYnMtY29udGVudCB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgcGFkZGluZzogM3JlbTtcclxuICAgIGZsZXg6IDEgMSBhdXRvO1xyXG4gICAgb3ZlcmZsb3cteDogaGlkZGVuO1xyXG4gICAgb3ZlcmZsb3cteTogb3ZlcmxheTtcclxuICB9XHJcbn1cclxuIl19 */"

/***/ }),

/***/ "./src/app/wallet/wallet.component.ts":
/*!********************************************!*\
  !*** ./src/app/wallet/wallet.component.ts ***!
  \********************************************/
/*! exports provided: WalletComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "WalletComponent", function() { return WalletComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};




var WalletComponent = /** @class */ (function () {
    function WalletComponent(route, router, backend, variablesService, ngZone) {
        this.route = route;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.tabs = [
            {
                title: 'WALLET.TABS.SEND',
                icon: 'send',
                link: '/send',
                indicator: false,
                active: true
            },
            {
                title: 'WALLET.TABS.RECEIVE',
                icon: 'receive',
                link: '/receive',
                indicator: false,
                active: false
            },
            {
                title: 'WALLET.TABS.HISTORY',
                icon: 'history',
                link: '/history',
                indicator: false,
                active: false
            },
            {
                title: 'WALLET.TABS.CONTRACTS',
                icon: 'contracts',
                link: '/contracts',
                indicator: 1,
                active: false
            },
            {
                title: 'WALLET.TABS.MESSAGES',
                icon: 'messages',
                link: '/messages',
                indicator: 32,
                active: false
            },
            {
                title: 'WALLET.TABS.STAKING',
                icon: 'staking',
                link: '/staking',
                indicator: false,
                active: false
            }
        ];
    }
    WalletComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.subRouting = this.route.params.subscribe(function (params) {
            _this.walletID = +params['id'];
            _this.variablesService.setCurrentWallet(_this.walletID);
            for (var i = 0; i < _this.tabs.length; i++) {
                _this.tabs[i].active = (_this.tabs[i].link === '/' + _this.route.snapshot.firstChild.url[0].path);
            }
        });
    };
    WalletComponent.prototype.changeTab = function (index) {
        if ((this.tabs[index].link === '/contracts' || this.tabs[index].link === '/staking') && this.variablesService.daemon_state !== 2) {
            return;
        }
        this.tabs.forEach(function (tab) {
            tab.active = false;
        });
        this.tabs[index].active = true;
        this.router.navigate(['wallet/' + this.walletID + this.tabs[index].link]);
    };
    WalletComponent.prototype.copyAddress = function () {
        this.backend.setClipboard(this.variablesService.currentWallet.address);
    };
    /*closeWallet() {
      this.backend.closeWallet(this.walletID, () => {
        for (let i = this.variablesService.wallets.length - 1; i >= 0; i--) {
          if (this.variablesService.wallets[i].wallet_id === this.walletID) {
            this.variablesService.wallets.splice(i, 1);
          }
        }
        this.backend.storeSecureAppData(() => {
          this.ngZone.run(() => {
            this.router.navigate(['/']);
          });
        });
      });
    }*/
    WalletComponent.prototype.ngOnDestroy = function () {
        this.subRouting.unsubscribe();
    };
    WalletComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-wallet',
            template: __webpack_require__(/*! ./wallet.component.html */ "./src/app/wallet/wallet.component.html"),
            styles: [__webpack_require__(/*! ./wallet.component.scss */ "./src/app/wallet/wallet.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_1__["ActivatedRoute"],
            _angular_router__WEBPACK_IMPORTED_MODULE_1__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], WalletComponent);
    return WalletComponent;
}());



/***/ }),

/***/ "./src/environments/environment.ts":
/*!*****************************************!*\
  !*** ./src/environments/environment.ts ***!
  \*****************************************/
/*! exports provided: environment */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "environment", function() { return environment; });
// This file can be replaced during build by using the `fileReplacements` array.
// `ng build --prod` replaces `environment.ts` with `environment.prod.ts`.
// The list of file replacements can be found in `angular.json`.
var environment = {
    production: false
};
/*
 * For easier debugging in development mode, you can import the following file
 * to ignore zone related error stack frames such as `zone.run`, `zoneDelegate.invokeTask`.
 *
 * This import should be commented out in production mode because it will have a negative impact
 * on performance if an error is thrown.
 */
// import 'zone.js/dist/zone-error';  // Included with Angular CLI.


/***/ }),

/***/ "./src/main.ts":
/*!*********************!*\
  !*** ./src/main.ts ***!
  \*********************/
/*! no exports provided */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_platform_browser_dynamic__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/platform-browser-dynamic */ "./node_modules/@angular/platform-browser-dynamic/fesm5/platform-browser-dynamic.js");
/* harmony import */ var _app_app_module__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ./app/app.module */ "./src/app/app.module.ts");
/* harmony import */ var _environments_environment__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ./environments/environment */ "./src/environments/environment.ts");




if (_environments_environment__WEBPACK_IMPORTED_MODULE_3__["environment"].production) {
    Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["enableProdMode"])();
}
Object(_angular_platform_browser_dynamic__WEBPACK_IMPORTED_MODULE_1__["platformBrowserDynamic"])().bootstrapModule(_app_app_module__WEBPACK_IMPORTED_MODULE_2__["AppModule"])
    .catch(function (err) { return console.error(err); });


/***/ }),

/***/ 0:
/*!***************************!*\
  !*** multi ./src/main.ts ***!
  \***************************/
/*! no static exports found */
/***/ (function(module, exports, __webpack_require__) {

module.exports = __webpack_require__(/*! D:\zano\src\main.ts */"./src/main.ts");


/***/ })

},[[0,"runtime","vendor"]]]);
//# sourceMappingURL=main.js.map