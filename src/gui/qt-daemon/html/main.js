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

/***/ "./src/app/_helpers/directives/input-disable-selection/input-disable-selection.directive.ts":
/*!**************************************************************************************************!*\
  !*** ./src/app/_helpers/directives/input-disable-selection/input-disable-selection.directive.ts ***!
  \**************************************************************************************************/
/*! exports provided: InputDisableSelectionDirective */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "InputDisableSelectionDirective", function() { return InputDisableSelectionDirective; });
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

var InputDisableSelectionDirective = /** @class */ (function () {
    function InputDisableSelectionDirective() {
    }
    InputDisableSelectionDirective.prototype.handleInput = function (event) {
        if (event.target.readOnly) {
            event.preventDefault();
        }
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["HostListener"])('mousedown', ['$event']),
        __metadata("design:type", Function),
        __metadata("design:paramtypes", [Event]),
        __metadata("design:returntype", void 0)
    ], InputDisableSelectionDirective.prototype, "handleInput", null);
    InputDisableSelectionDirective = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Directive"])({
            selector: 'input'
        }),
        __metadata("design:paramtypes", [])
    ], InputDisableSelectionDirective);
    return InputDisableSelectionDirective;
}());



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
/* harmony import */ var _services_variables_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../../services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
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
    function InputValidateDirective(el, variablesService) {
        this.el = el;
        this.variablesService = variablesService;
    }
    Object.defineProperty(InputValidateDirective.prototype, "defineInputType", {
        set: function (type) {
            this.type = type;
        },
        enumerable: true,
        configurable: true
    });
    InputValidateDirective.prototype.handleInput = function (event) {
        if (this.type === 'money') {
            this.moneyValidation(event);
        }
        else if (this.type === 'integer') {
            this.integerValidation(event);
        }
    };
    InputValidateDirective.prototype.moneyValidation = function (event) {
        var currentValue = event.target.value;
        var originalValue = currentValue;
        var OnlyD = /[^\d\.]/g;
        var _has_error = currentValue.match(OnlyD);
        if (_has_error && _has_error.length) {
            currentValue = currentValue.replace(',', '.').replace(OnlyD, '');
        }
        var _double_separator = currentValue.match(/\./g);
        if (_double_separator && _double_separator.length > 1) {
            currentValue = currentValue.substr(0, currentValue.lastIndexOf('.'));
        }
        if (currentValue.indexOf('.') === 0) {
            currentValue = '0' + currentValue;
        }
        var _zero_fill = currentValue.split('.');
        if (_zero_fill[0].length > 7) {
            _zero_fill[0] = _zero_fill[0].substr(0, 7);
        }
        if (1 in _zero_fill && _zero_fill[1].length) {
            _zero_fill[1] = _zero_fill[1].substr(0, this.variablesService.digits);
        }
        currentValue = _zero_fill.join('.');
        if (currentValue !== originalValue) {
            var cursorPosition = event.target.selectionEnd;
            event.target.value = currentValue;
            event.target.setSelectionRange(cursorPosition, cursorPosition);
            event.target.dispatchEvent(new Event('input'));
        }
    };
    InputValidateDirective.prototype.integerValidation = function (event) {
        var currentValue = event.target.value;
        var originalValue = currentValue;
        var OnlyD = /[^\d]/g;
        var _has_error = currentValue.match(OnlyD);
        if (_has_error && _has_error.length) {
            currentValue = currentValue.replace(OnlyD, '');
        }
        if (currentValue !== originalValue) {
            var cursorPosition = event.target.selectionEnd;
            event.target.value = currentValue;
            event.target.setSelectionRange(cursorPosition, cursorPosition);
        }
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])('appInputValidate'),
        __metadata("design:type", String),
        __metadata("design:paramtypes", [String])
    ], InputValidateDirective.prototype, "defineInputType", null);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["HostListener"])('input', ['$event']),
        __metadata("design:type", Function),
        __metadata("design:paramtypes", [Event]),
        __metadata("design:returntype", void 0)
    ], InputValidateDirective.prototype, "handleInput", null);
    InputValidateDirective = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Directive"])({
            selector: '[appInputValidate]'
        }),
        __metadata("design:paramtypes", [_angular_core__WEBPACK_IMPORTED_MODULE_0__["ElementRef"], _services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"]])
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

module.exports = "<div class=\"modal\">\n  <div class=\"content\">\n    <i class=\"icon\" [class.error]=\"type === 'error'\" [class.success]=\"type === 'success'\" [class.info]=\"type === 'info'\"></i>\n    <div class=\"message-container\">\n      <span class=\"title\">{{title}}</span>\n      <span class=\"message\" [innerHTML]=\"message\"></span>\n    </div>\n  </div>\n  <button type=\"button\" class=\"action-button\" (click)=\"onClose()\" #btn>{{ 'MODALS.OK' | translate }}</button>\n  <button type=\"button\" class=\"close-button\" (click)=\"onClose()\"><i class=\"icon close\"></i></button>\n</div>\n"

/***/ }),

/***/ "./src/app/_helpers/directives/modal-container/modal-container.component.scss":
/*!************************************************************************************!*\
  !*** ./src/app/_helpers/directives/modal-container/modal-container.component.scss ***!
  \************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: fixed;\n  top: 0;\n  bottom: 0;\n  left: 0;\n  right: 0;\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  -webkit-box-pack: center;\n          justify-content: center;\n  background: rgba(255, 255, 255, 0.25); }\n\n.modal {\n  position: relative;\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  background-position: center;\n  background-size: 200%;\n  padding: 2rem;\n  width: 34rem; }\n\n.modal .content {\n    display: -webkit-box;\n    display: flex;\n    margin: 1.2rem 0; }\n\n.modal .content .icon {\n      -webkit-box-flex: 0;\n              flex: 0 0 auto;\n      width: 4.4rem;\n      height: 4.4rem; }\n\n.modal .content .icon.error {\n        -webkit-mask: url('modal-alert.svg') no-repeat center;\n                mask: url('modal-alert.svg') no-repeat center; }\n\n.modal .content .icon.success {\n        -webkit-mask: url('modal-success.svg') no-repeat center;\n                mask: url('modal-success.svg') no-repeat center; }\n\n.modal .content .icon.info {\n        -webkit-mask: url('modal-info.svg') no-repeat center;\n                mask: url('modal-info.svg') no-repeat center; }\n\n.modal .content .message-container {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-orient: vertical;\n      -webkit-box-direction: normal;\n              flex-direction: column;\n      -webkit-box-align: start;\n              align-items: flex-start;\n      -webkit-box-pack: center;\n              justify-content: center;\n      margin-left: 2rem; }\n\n.modal .content .message-container .title {\n        font-size: 1.8rem;\n        font-weight: 600;\n        line-height: 2.2rem; }\n\n.modal .content .message-container .message {\n        font-size: 1.3rem;\n        line-height: 1.8rem;\n        margin-top: 0.4rem; }\n\n.modal .action-button {\n    margin: 1.2rem auto 0.6rem;\n    width: 10rem;\n    height: 2.4rem; }\n\n.modal .close-button {\n    position: absolute;\n    top: 0;\n    right: 0;\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: center;\n            justify-content: center;\n    background: transparent;\n    margin: 0;\n    padding: 0;\n    width: 2.4rem;\n    height: 2.4rem; }\n\n.modal .close-button .icon {\n      -webkit-mask: url('close.svg') no-repeat center;\n              mask: url('close.svg') no-repeat center;\n      width: 2.4rem;\n      height: 2.4rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL19oZWxwZXJzL2RpcmVjdGl2ZXMvbW9kYWwtY29udGFpbmVyL21vZGFsLWNvbnRhaW5lci5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGVBQWU7RUFDZixNQUFNO0VBQ04sU0FBUztFQUNULE9BQU87RUFDUCxRQUFRO0VBQ1Isb0JBQWE7RUFBYixhQUFhO0VBQ2IseUJBQW1CO1VBQW5CLG1CQUFtQjtFQUNuQix3QkFBdUI7VUFBdkIsdUJBQXVCO0VBQ3ZCLHFDQUFxQyxFQUFBOztBQUV2QztFQUNFLGtCQUFrQjtFQUNsQixvQkFBYTtFQUFiLGFBQWE7RUFDYiw0QkFBc0I7RUFBdEIsNkJBQXNCO1VBQXRCLHNCQUFzQjtFQUN0QiwyQkFBMkI7RUFDM0IscUJBQXFCO0VBQ3JCLGFBQWE7RUFDYixZQUFZLEVBQUE7O0FBUGQ7SUFVSSxvQkFBYTtJQUFiLGFBQWE7SUFDYixnQkFBZ0IsRUFBQTs7QUFYcEI7TUFjTSxtQkFBYztjQUFkLGNBQWM7TUFDZCxhQUFhO01BQ2IsY0FBYyxFQUFBOztBQWhCcEI7UUFtQlEscURBQTZEO2dCQUE3RCw2Q0FBNkQsRUFBQTs7QUFuQnJFO1FBdUJRLHVEQUErRDtnQkFBL0QsK0NBQStELEVBQUE7O0FBdkJ2RTtRQTJCUSxvREFBNEQ7Z0JBQTVELDRDQUE0RCxFQUFBOztBQTNCcEU7TUFnQ00sb0JBQWE7TUFBYixhQUFhO01BQ2IsNEJBQXNCO01BQXRCLDZCQUFzQjtjQUF0QixzQkFBc0I7TUFDdEIsd0JBQXVCO2NBQXZCLHVCQUF1QjtNQUN2Qix3QkFBdUI7Y0FBdkIsdUJBQXVCO01BQ3ZCLGlCQUFpQixFQUFBOztBQXBDdkI7UUF1Q1EsaUJBQWlCO1FBQ2pCLGdCQUFnQjtRQUNoQixtQkFBbUIsRUFBQTs7QUF6QzNCO1FBNkNRLGlCQUFpQjtRQUNqQixtQkFBbUI7UUFDbkIsa0JBQWtCLEVBQUE7O0FBL0MxQjtJQXFESSwwQkFBMEI7SUFDMUIsWUFBWTtJQUNaLGNBQWMsRUFBQTs7QUF2RGxCO0lBMkRJLGtCQUFrQjtJQUNsQixNQUFNO0lBQ04sUUFBUTtJQUNSLG9CQUFhO0lBQWIsYUFBYTtJQUNiLHlCQUFtQjtZQUFuQixtQkFBbUI7SUFDbkIsd0JBQXVCO1lBQXZCLHVCQUF1QjtJQUN2Qix1QkFBdUI7SUFDdkIsU0FBUztJQUNULFVBQVU7SUFDVixhQUFhO0lBQ2IsY0FBYyxFQUFBOztBQXJFbEI7TUF3RU0sK0NBQXVEO2NBQXZELHVDQUF1RDtNQUN2RCxhQUFhO01BQ2IsY0FBYyxFQUFBIiwiZmlsZSI6InNyYy9hcHAvX2hlbHBlcnMvZGlyZWN0aXZlcy9tb2RhbC1jb250YWluZXIvbW9kYWwtY29udGFpbmVyLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xuICBwb3NpdGlvbjogZml4ZWQ7XG4gIHRvcDogMDtcbiAgYm90dG9tOiAwO1xuICBsZWZ0OiAwO1xuICByaWdodDogMDtcbiAgZGlzcGxheTogZmxleDtcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XG4gIGJhY2tncm91bmQ6IHJnYmEoMjU1LCAyNTUsIDI1NSwgMC4yNSk7XG59XG4ubW9kYWwge1xuICBwb3NpdGlvbjogcmVsYXRpdmU7XG4gIGRpc3BsYXk6IGZsZXg7XG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XG4gIGJhY2tncm91bmQtcG9zaXRpb246IGNlbnRlcjtcbiAgYmFja2dyb3VuZC1zaXplOiAyMDAlO1xuICBwYWRkaW5nOiAycmVtO1xuICB3aWR0aDogMzRyZW07XG5cbiAgLmNvbnRlbnQge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgbWFyZ2luOiAxLjJyZW0gMDtcblxuICAgIC5pY29uIHtcbiAgICAgIGZsZXg6IDAgMCBhdXRvO1xuICAgICAgd2lkdGg6IDQuNHJlbTtcbiAgICAgIGhlaWdodDogNC40cmVtO1xuXG4gICAgICAmLmVycm9yIHtcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL21vZGFsLWFsZXJ0LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgIH1cblxuICAgICAgJi5zdWNjZXNzIHtcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL21vZGFsLXN1Y2Nlc3Muc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xuICAgICAgfVxuXG4gICAgICAmLmluZm8ge1xuICAgICAgICBtYXNrOiB1cmwofnNyYy9hc3NldHMvaWNvbnMvbW9kYWwtaW5mby5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICB9XG4gICAgfVxuXG4gICAgLm1lc3NhZ2UtY29udGFpbmVyIHtcbiAgICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xuICAgICAgYWxpZ24taXRlbXM6IGZsZXgtc3RhcnQ7XG4gICAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcbiAgICAgIG1hcmdpbi1sZWZ0OiAycmVtO1xuXG4gICAgICAudGl0bGUge1xuICAgICAgICBmb250LXNpemU6IDEuOHJlbTtcbiAgICAgICAgZm9udC13ZWlnaHQ6IDYwMDtcbiAgICAgICAgbGluZS1oZWlnaHQ6IDIuMnJlbTtcbiAgICAgIH1cblxuICAgICAgLm1lc3NhZ2Uge1xuICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICAgICAgbGluZS1oZWlnaHQ6IDEuOHJlbTtcbiAgICAgICAgbWFyZ2luLXRvcDogMC40cmVtO1xuICAgICAgfVxuICAgIH1cbiAgfVxuXG4gIC5hY3Rpb24tYnV0dG9uIHtcbiAgICBtYXJnaW46IDEuMnJlbSBhdXRvIDAuNnJlbTtcbiAgICB3aWR0aDogMTByZW07XG4gICAgaGVpZ2h0OiAyLjRyZW07XG4gIH1cblxuICAuY2xvc2UtYnV0dG9uIHtcbiAgICBwb3NpdGlvbjogYWJzb2x1dGU7XG4gICAgdG9wOiAwO1xuICAgIHJpZ2h0OiAwO1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcbiAgICBiYWNrZ3JvdW5kOiB0cmFuc3BhcmVudDtcbiAgICBtYXJnaW46IDA7XG4gICAgcGFkZGluZzogMDtcbiAgICB3aWR0aDogMi40cmVtO1xuICAgIGhlaWdodDogMi40cmVtO1xuXG4gICAgLmljb24ge1xuICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL2Nsb3NlLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgIHdpZHRoOiAyLjRyZW07XG4gICAgICBoZWlnaHQ6IDIuNHJlbTtcbiAgICB9XG4gIH1cbn1cbiJdfQ== */"

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
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
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
    function ModalContainerComponent(translate) {
        this.translate = translate;
        this.close = new _angular_core__WEBPACK_IMPORTED_MODULE_0__["EventEmitter"]();
    }
    ModalContainerComponent.prototype.ngOnInit = function () {
        this.button.nativeElement.focus();
        switch (this.type) {
            case 'error':
                this.title = this.translate.instant('MODALS.ERROR');
                break;
            case 'success':
                this.title = this.translate.instant('MODALS.SUCCESS');
                break;
            case 'info':
                this.title = this.translate.instant('MODALS.INFO');
                break;
        }
    };
    ModalContainerComponent.prototype.onClose = function () {
        this.close.emit();
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", String)
    ], ModalContainerComponent.prototype, "type", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", String)
    ], ModalContainerComponent.prototype, "message", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Output"])(),
        __metadata("design:type", Object)
    ], ModalContainerComponent.prototype, "close", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["ViewChild"])('btn'),
        __metadata("design:type", _angular_core__WEBPACK_IMPORTED_MODULE_0__["ElementRef"])
    ], ModalContainerComponent.prototype, "button", void 0);
    ModalContainerComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-modal-container',
            template: __webpack_require__(/*! ./modal-container.component.html */ "./src/app/_helpers/directives/modal-container/modal-container.component.html"),
            styles: [__webpack_require__(/*! ./modal-container.component.scss */ "./src/app/_helpers/directives/modal-container/modal-container.component.scss")]
        }),
        __metadata("design:paramtypes", [_ngx_translate_core__WEBPACK_IMPORTED_MODULE_1__["TranslateService"]])
    ], ModalContainerComponent);
    return ModalContainerComponent;
}());



/***/ }),

/***/ "./src/app/_helpers/directives/progress-container/progress-container.component.html":
/*!******************************************************************************************!*\
  !*** ./src/app/_helpers/directives/progress-container/progress-container.component.html ***!
  \******************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"progress-bar-container\">\n  <div class=\"progress-bar\">\n    <div class=\"progress-bar-full\" [style.width]=\"width\"></div>\n  </div>\n  <div class=\"progress-labels\">\n    <span *ngFor=\"let label of labels\">\n      {{ label | translate }}\n    </span>\n  </div>\n</div>\n"

/***/ }),

/***/ "./src/app/_helpers/directives/progress-container/progress-container.component.scss":
/*!******************************************************************************************!*\
  !*** ./src/app/_helpers/directives/progress-container/progress-container.component.scss ***!
  \******************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".progress-bar-container {\n  position: absolute;\n  bottom: 0;\n  left: 0;\n  padding: 0 3rem;\n  width: 100%;\n  height: 3rem; }\n  .progress-bar-container .progress-bar {\n    position: absolute;\n    top: -0.7rem;\n    left: 0;\n    margin: 0 3rem;\n    width: calc(100% - 6rem);\n    height: 0.7rem; }\n  .progress-bar-container .progress-bar .progress-bar-full {\n      height: 0.7rem; }\n  .progress-bar-container .progress-labels {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: justify;\n            justify-content: space-between;\n    font-size: 1.2rem;\n    height: 100%; }\n  .progress-bar-container .progress-labels span {\n      -webkit-box-flex: 1;\n              flex: 1 0 0;\n      text-align: center; }\n  .progress-bar-container .progress-labels span:first-child {\n        text-align: left; }\n  .progress-bar-container .progress-labels span:last-child {\n        text-align: right; }\n  .progress-bar-container .progress-time {\n    position: absolute;\n    top: -3rem;\n    left: 50%;\n    -webkit-transform: translateX(-50%);\n            transform: translateX(-50%);\n    font-size: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL19oZWxwZXJzL2RpcmVjdGl2ZXMvcHJvZ3Jlc3MtY29udGFpbmVyL3Byb2dyZXNzLWNvbnRhaW5lci5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGtCQUFrQjtFQUNsQixTQUFTO0VBQ1QsT0FBTztFQUNQLGVBQWU7RUFDZixXQUFXO0VBQ1gsWUFBWSxFQUFBO0VBTmQ7SUFTSSxrQkFBa0I7SUFDbEIsWUFBWTtJQUNaLE9BQU87SUFDUCxjQUFjO0lBQ2Qsd0JBQXdCO0lBQ3hCLGNBQWMsRUFBQTtFQWRsQjtNQWlCTSxjQUFjLEVBQUE7RUFqQnBCO0lBc0JJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLHlCQUFtQjtZQUFuQixtQkFBbUI7SUFDbkIseUJBQThCO1lBQTlCLDhCQUE4QjtJQUM5QixpQkFBaUI7SUFDakIsWUFBWSxFQUFBO0VBMUJoQjtNQTZCTSxtQkFBVztjQUFYLFdBQVc7TUFDWCxrQkFBa0IsRUFBQTtFQTlCeEI7UUFpQ1EsZ0JBQWdCLEVBQUE7RUFqQ3hCO1FBcUNRLGlCQUFpQixFQUFBO0VBckN6QjtJQTJDSSxrQkFBa0I7SUFDbEIsVUFBVTtJQUNWLFNBQVM7SUFDVCxtQ0FBMkI7WUFBM0IsMkJBQTJCO0lBQzNCLGlCQUFpQixFQUFBIiwiZmlsZSI6InNyYy9hcHAvX2hlbHBlcnMvZGlyZWN0aXZlcy9wcm9ncmVzcy1jb250YWluZXIvcHJvZ3Jlc3MtY29udGFpbmVyLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLnByb2dyZXNzLWJhci1jb250YWluZXIge1xuICBwb3NpdGlvbjogYWJzb2x1dGU7XG4gIGJvdHRvbTogMDtcbiAgbGVmdDogMDtcbiAgcGFkZGluZzogMCAzcmVtO1xuICB3aWR0aDogMTAwJTtcbiAgaGVpZ2h0OiAzcmVtO1xuXG4gIC5wcm9ncmVzcy1iYXIge1xuICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcbiAgICB0b3A6IC0wLjdyZW07XG4gICAgbGVmdDogMDtcbiAgICBtYXJnaW46IDAgM3JlbTtcbiAgICB3aWR0aDogY2FsYygxMDAlIC0gNnJlbSk7XG4gICAgaGVpZ2h0OiAwLjdyZW07XG5cbiAgICAucHJvZ3Jlc3MtYmFyLWZ1bGwge1xuICAgICAgaGVpZ2h0OiAwLjdyZW07XG4gICAgfVxuICB9XG5cbiAgLnByb2dyZXNzLWxhYmVscyB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcbiAgICBmb250LXNpemU6IDEuMnJlbTtcbiAgICBoZWlnaHQ6IDEwMCU7XG5cbiAgICBzcGFuIHtcbiAgICAgIGZsZXg6IDEgMCAwO1xuICAgICAgdGV4dC1hbGlnbjogY2VudGVyO1xuXG4gICAgICAmOmZpcnN0LWNoaWxkIHtcbiAgICAgICAgdGV4dC1hbGlnbjogbGVmdDtcbiAgICAgIH1cblxuICAgICAgJjpsYXN0LWNoaWxkIHtcbiAgICAgICAgdGV4dC1hbGlnbjogcmlnaHQ7XG4gICAgICB9XG4gICAgfVxuICB9XG5cbiAgLnByb2dyZXNzLXRpbWUge1xuICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcbiAgICB0b3A6IC0zcmVtO1xuICAgIGxlZnQ6IDUwJTtcbiAgICB0cmFuc2Zvcm06IHRyYW5zbGF0ZVgoLTUwJSk7XG4gICAgZm9udC1zaXplOiAxLjJyZW07XG4gIH1cbn1cbiJdfQ== */"

/***/ }),

/***/ "./src/app/_helpers/directives/progress-container/progress-container.component.ts":
/*!****************************************************************************************!*\
  !*** ./src/app/_helpers/directives/progress-container/progress-container.component.ts ***!
  \****************************************************************************************/
/*! exports provided: ProgressContainerComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ProgressContainerComponent", function() { return ProgressContainerComponent; });
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

var ProgressContainerComponent = /** @class */ (function () {
    function ProgressContainerComponent() {
    }
    ProgressContainerComponent.prototype.ngOnInit = function () { };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", String)
    ], ProgressContainerComponent.prototype, "width", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", Array)
    ], ProgressContainerComponent.prototype, "labels", void 0);
    ProgressContainerComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-progress-container',
            template: __webpack_require__(/*! ./progress-container.component.html */ "./src/app/_helpers/directives/progress-container/progress-container.component.html"),
            styles: [__webpack_require__(/*! ./progress-container.component.scss */ "./src/app/_helpers/directives/progress-container/progress-container.component.scss")]
        }),
        __metadata("design:paramtypes", [])
    ], ProgressContainerComponent);
    return ProgressContainerComponent;
}());



/***/ }),

/***/ "./src/app/_helpers/directives/staking-switch/staking-switch.component.html":
/*!**********************************************************************************!*\
  !*** ./src/app/_helpers/directives/staking-switch/staking-switch.component.html ***!
  \**********************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"switch\" (click)=\"toggleStaking(); $event.stopPropagation()\">\n  <span class=\"option\" *ngIf=\"staking\">{{ 'STAKING.SWITCH.ON' | translate }}</span>\n  <span class=\"circle\" [class.on]=\"staking\" [class.off]=\"!staking\"></span>\n  <span class=\"option\" *ngIf=\"!staking\">{{ 'STAKING.SWITCH.OFF' | translate }}</span>\n</div>\n"

/***/ }),

/***/ "./src/app/_helpers/directives/staking-switch/staking-switch.component.scss":
/*!**********************************************************************************!*\
  !*** ./src/app/_helpers/directives/staking-switch/staking-switch.component.scss ***!
  \**********************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".switch {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  -webkit-box-pack: justify;\n          justify-content: space-between;\n  border-radius: 1rem;\n  cursor: pointer;\n  font-size: 1rem;\n  padding: 0.5rem;\n  width: 5rem;\n  height: 2rem; }\n  .switch .circle {\n    border-radius: 1rem;\n    width: 1.2rem;\n    height: 1.2rem; }\n  .switch .option {\n    margin: 0 0.2rem;\n    line-height: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL19oZWxwZXJzL2RpcmVjdGl2ZXMvc3Rha2luZy1zd2l0Y2gvc3Rha2luZy1zd2l0Y2guY29tcG9uZW50LnNjc3MiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IkFBQUE7RUFDRSxvQkFBYTtFQUFiLGFBQWE7RUFDYix5QkFBbUI7VUFBbkIsbUJBQW1CO0VBQ25CLHlCQUE4QjtVQUE5Qiw4QkFBOEI7RUFDOUIsbUJBQW1CO0VBQ25CLGVBQWU7RUFDZixlQUFlO0VBQ2YsZUFBZTtFQUNmLFdBQVc7RUFDWCxZQUFZLEVBQUE7RUFUZDtJQVlJLG1CQUFtQjtJQUNuQixhQUFhO0lBQ2IsY0FBYyxFQUFBO0VBZGxCO0lBa0JJLGdCQUFnQjtJQUNoQixtQkFBbUIsRUFBQSIsImZpbGUiOiJzcmMvYXBwL19oZWxwZXJzL2RpcmVjdGl2ZXMvc3Rha2luZy1zd2l0Y2gvc3Rha2luZy1zd2l0Y2guY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyIuc3dpdGNoIHtcbiAgZGlzcGxheTogZmxleDtcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAganVzdGlmeS1jb250ZW50OiBzcGFjZS1iZXR3ZWVuO1xuICBib3JkZXItcmFkaXVzOiAxcmVtO1xuICBjdXJzb3I6IHBvaW50ZXI7XG4gIGZvbnQtc2l6ZTogMXJlbTtcbiAgcGFkZGluZzogMC41cmVtO1xuICB3aWR0aDogNXJlbTtcbiAgaGVpZ2h0OiAycmVtO1xuXG4gIC5jaXJjbGUge1xuICAgIGJvcmRlci1yYWRpdXM6IDFyZW07XG4gICAgd2lkdGg6IDEuMnJlbTtcbiAgICBoZWlnaHQ6IDEuMnJlbTtcbiAgfVxuXG4gIC5vcHRpb24ge1xuICAgIG1hcmdpbjogMCAwLjJyZW07XG4gICAgbGluZS1oZWlnaHQ6IDEuMnJlbTtcbiAgfVxufVxuIl19 */"

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
    StakingSwitchComponent.prototype.ngOnInit = function () { };
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


var TooltipDirective = /** @class */ (function () {
    function TooltipDirective(el, renderer, route) {
        this.el = el;
        this.renderer = renderer;
        this.route = route;
        this.timeout = 0;
        this.delay = 0;
        this.showWhenNoOverflow = true;
        this.onHide = new _angular_core__WEBPACK_IMPORTED_MODULE_0__["EventEmitter"]();
    }
    TooltipDirective.prototype.onMouseEnter = function () {
        if (this.showWhenNoOverflow || (!this.showWhenNoOverflow && this.el.nativeElement.offsetWidth < this.el.nativeElement.scrollWidth)) {
            this.cursor = 'pointer';
            if (!this.tooltip) {
                this.show();
            }
            else {
                this.cancelHide();
            }
        }
    };
    TooltipDirective.prototype.onMouseLeave = function () {
        if (this.tooltip) {
            this.hide();
        }
    };
    TooltipDirective.prototype.show = function () {
        this.create();
        this.placement = this.placement === null ? 'top' : this.placement;
        this.setPosition(this.placement);
    };
    TooltipDirective.prototype.hide = function () {
        var _this = this;
        this.removeTooltipTimeout = setTimeout(function () {
            _this.renderer.setStyle(_this.tooltip, 'opacity', '0');
            _this.removeTooltipTimeoutInner = setTimeout(function () {
                _this.renderer.removeChild(document.body, _this.tooltip);
                _this.tooltip.removeEventListener('mouseenter', _this.enter);
                _this.tooltip.removeEventListener('mouseleave', _this.leave);
                _this.tooltip = null;
                _this.onHide.emit(true);
            }, _this.delay);
        }, this.timeout);
    };
    TooltipDirective.prototype.cancelHide = function () {
        clearTimeout(this.removeTooltipTimeout);
        clearTimeout(this.removeTooltipTimeoutInner);
        this.renderer.setStyle(this.tooltip, 'opacity', '1');
    };
    TooltipDirective.prototype.create = function () {
        var _this = this;
        this.tooltip = this.renderer.createElement('div');
        var innerBlock = this.renderer.createElement('div');
        if (typeof this.tooltipInner === 'string') {
            innerBlock.innerHTML = this.tooltipInner;
        }
        else {
            innerBlock = this.tooltipInner;
        }
        this.renderer.addClass(innerBlock, 'tooltip-inner');
        this.renderer.addClass(innerBlock, 'scrolled-content');
        this.renderer.appendChild(this.tooltip, innerBlock);
        this.renderer.appendChild(document.body, this.tooltip);
        this.enter = function () {
            _this.cancelHide();
        };
        this.tooltip.addEventListener('mouseenter', this.enter);
        this.leave = function () {
            if (_this.tooltip) {
                _this.hide();
            }
        };
        this.tooltip.addEventListener('mouseleave', this.leave);
        this.renderer.setStyle(document.body, 'position', 'relative');
        this.renderer.setStyle(this.tooltip, 'position', 'absolute');
        if (this.tooltipClass !== null) {
            var classes = this.tooltipClass.split(' ');
            for (var i = 0; i < classes.length; i++) {
                this.renderer.addClass(this.tooltip, classes[i]);
            }
        }
        this.renderer.setStyle(this.tooltip, 'opacity', '0');
        this.renderer.setStyle(this.tooltip, '-webkit-transition', "opacity " + this.delay + "ms");
        this.renderer.setStyle(this.tooltip, '-moz-transition', "opacity " + this.delay + "ms");
        this.renderer.setStyle(this.tooltip, '-o-transition', "opacity " + this.delay + "ms");
        this.renderer.setStyle(this.tooltip, 'transition', "opacity " + this.delay + "ms");
        window.setTimeout(function () {
            _this.renderer.setStyle(_this.tooltip, 'opacity', '1');
        }, 0);
    };
    TooltipDirective.prototype.setPosition = function (placement) {
        var hostPos = this.el.nativeElement.getBoundingClientRect();
        this.renderer.addClass(this.tooltip, 'ng-tooltip-' + placement);
        var topExit = hostPos.top - this.tooltip.getBoundingClientRect().height - parseInt(getComputedStyle(this.tooltip).marginTop, 10) < 0;
        var bottomExit = window.innerHeight < hostPos.bottom + this.tooltip.getBoundingClientRect().height + parseInt(getComputedStyle(this.tooltip).marginTop, 10);
        switch (placement) {
            case 'top':
                if (topExit) {
                    this.renderer.removeClass(this.tooltip, 'ng-tooltip-' + placement);
                    this.setPosition('bottom');
                    return;
                }
                else {
                    this.renderer.setStyle(this.tooltip, 'left', hostPos.left + (hostPos.right - hostPos.left) / 2 - this.tooltip.getBoundingClientRect().width / 2 + 'px');
                    this.renderer.setStyle(this.tooltip, 'top', hostPos.top - this.tooltip.getBoundingClientRect().height + 'px');
                    this.checkSides();
                }
                break;
            case 'top-left':
                if (topExit) {
                    this.renderer.removeClass(this.tooltip, 'ng-tooltip-' + placement);
                    this.setPosition('bottom-left');
                    return;
                }
                else {
                    this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
                    this.renderer.setStyle(this.tooltip, 'top', hostPos.top - this.tooltip.getBoundingClientRect().height + 'px');
                    this.checkSides();
                }
                break;
            case 'top-right':
                if (topExit) {
                    this.renderer.removeClass(this.tooltip, 'ng-tooltip-' + placement);
                    this.setPosition('bottom-right');
                    return;
                }
                else {
                    this.renderer.setStyle(this.tooltip, 'left', hostPos.right - this.tooltip.offsetWidth + 'px');
                    this.renderer.setStyle(this.tooltip, 'top', hostPos.top - this.tooltip.getBoundingClientRect().height + 'px');
                    this.checkSides();
                }
                break;
            case 'bottom':
                if (bottomExit) {
                    this.renderer.removeClass(this.tooltip, 'ng-tooltip-' + placement);
                    this.setPosition('top');
                    return;
                }
                else {
                    this.renderer.setStyle(this.tooltip, 'top', hostPos.bottom + 'px');
                    this.renderer.setStyle(this.tooltip, 'left', hostPos.left + (hostPos.right - hostPos.left) / 2 - this.tooltip.getBoundingClientRect().width / 2 + 'px');
                    this.checkSides();
                }
                break;
            case 'bottom-left':
                if (bottomExit) {
                    this.renderer.removeClass(this.tooltip, 'ng-tooltip-' + placement);
                    this.setPosition('top-left');
                    return;
                }
                else {
                    this.renderer.setStyle(this.tooltip, 'top', hostPos.bottom + 'px');
                    this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
                    this.checkSides();
                }
                break;
            case 'bottom-right':
                if (bottomExit) {
                    this.renderer.removeClass(this.tooltip, 'ng-tooltip-' + placement);
                    this.setPosition('top-right');
                    return;
                }
                else {
                    this.renderer.setStyle(this.tooltip, 'top', hostPos.bottom + 'px');
                    this.renderer.setStyle(this.tooltip, 'left', hostPos.right - this.tooltip.offsetWidth + 'px');
                    this.checkSides();
                }
                break;
            case 'left':
                this.renderer.setStyle(this.tooltip, 'left', hostPos.left - this.tooltip.getBoundingClientRect().width + 'px');
                this.renderer.setStyle(this.tooltip, 'top', hostPos.top + (hostPos.bottom - hostPos.top) / 2 - this.tooltip.getBoundingClientRect().height / 2 + 'px');
                break;
            case 'left-top':
                this.renderer.setStyle(this.tooltip, 'top', hostPos.top + 'px');
                this.renderer.setStyle(this.tooltip, 'left', hostPos.left - this.tooltip.getBoundingClientRect().width + 'px');
                break;
            case 'left-bottom':
                this.renderer.setStyle(this.tooltip, 'left', hostPos.left - this.tooltip.getBoundingClientRect().width + 'px');
                this.renderer.setStyle(this.tooltip, 'top', hostPos.bottom - this.tooltip.getBoundingClientRect().height + 'px');
                break;
            case 'right':
                this.renderer.setStyle(this.tooltip, 'left', hostPos.right + 'px');
                this.renderer.setStyle(this.tooltip, 'top', hostPos.top + (hostPos.bottom - hostPos.top) / 2 - this.tooltip.getBoundingClientRect().height / 2 + 'px');
                break;
            case 'right-top':
                this.renderer.setStyle(this.tooltip, 'top', hostPos.top + 'px');
                this.renderer.setStyle(this.tooltip, 'left', hostPos.right + 'px');
                break;
            case 'right-bottom':
                this.renderer.setStyle(this.tooltip, 'left', hostPos.right + 'px');
                this.renderer.setStyle(this.tooltip, 'top', hostPos.bottom - this.tooltip.getBoundingClientRect().height + 'px');
                break;
        }
    };
    TooltipDirective.prototype.checkSides = function () {
        if (this.tooltip.getBoundingClientRect().left < 0) {
            this.renderer.setStyle(this.tooltip, 'left', 0);
        }
        if (this.tooltip.getBoundingClientRect().right > window.innerWidth) {
            this.renderer.setStyle(this.tooltip, 'left', window.innerWidth - this.tooltip.getBoundingClientRect().width + 'px');
        }
    };
    TooltipDirective.prototype.ngOnDestroy = function () {
        clearTimeout(this.removeTooltipTimeout);
        clearTimeout(this.removeTooltipTimeoutInner);
        if (this.tooltip) {
            this.renderer.removeChild(document.body, this.tooltip);
            this.tooltip = null;
        }
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["HostBinding"])('style.cursor'),
        __metadata("design:type", Object)
    ], TooltipDirective.prototype, "cursor", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])('tooltip'),
        __metadata("design:type", Object)
    ], TooltipDirective.prototype, "tooltipInner", void 0);
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
        __metadata("design:type", Object)
    ], TooltipDirective.prototype, "timeout", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", Object)
    ], TooltipDirective.prototype, "delay", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", Object)
    ], TooltipDirective.prototype, "showWhenNoOverflow", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Output"])(),
        __metadata("design:type", Object)
    ], TooltipDirective.prototype, "onHide", void 0);
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
            selector: '[tooltip]'
        }),
        __metadata("design:paramtypes", [_angular_core__WEBPACK_IMPORTED_MODULE_0__["ElementRef"], _angular_core__WEBPACK_IMPORTED_MODULE_0__["Renderer2"], _angular_router__WEBPACK_IMPORTED_MODULE_1__["ActivatedRoute"]])
    ], TooltipDirective);
    return TooltipDirective;
}());



/***/ }),

/***/ "./src/app/_helpers/directives/transaction-details/transaction-details.component.html":
/*!********************************************************************************************!*\
  !*** ./src/app/_helpers/directives/transaction-details/transaction-details.component.html ***!
  \********************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"table\">\n  <div class=\"row\">\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.ID' | translate }}</span>\n    <span class=\"cell key-value\" [style.flex-basis]=\"sizes[1] + 'px'\" (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, transaction.tx_hash)\" (click)=\"openInBrowser(transaction.tx_hash)\">{{transaction.tx_hash}}</span>\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[2] + 'px'\">{{ 'HISTORY.DETAILS.SIZE' | translate }}</span>\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[3] + 'px'\">{{ 'HISTORY.DETAILS.SIZE_VALUE' | translate : {value: transaction.tx_blob_size} }}</span>\n  </div>\n  <div class=\"row\">\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.HEIGHT' | translate }}</span>\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[1] + 'px'\">{{transaction.height}}</span>\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[2] + 'px'\">{{ 'HISTORY.DETAILS.CONFIRMATION' | translate }}</span>\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[3] + 'px'\">{{transaction.height === 0 ? 0 : variablesService.height_app - transaction.height}}</span>\n  </div>\n  <div class=\"row\">\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.INPUTS' | translate }}</span>\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[1] + 'px'\" tooltip=\"{{inputs.join(', ')}}\" placement=\"top\" tooltipClass=\"table-tooltip table-tooltip-dimensions\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{inputs.join(', ')}}</span>\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[2] + 'px'\">{{ 'HISTORY.DETAILS.OUTPUTS' | translate }}</span>\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[3] + 'px'\" tooltip=\"{{outputs.join(', ')}}\" placement=\"top\" tooltipClass=\"table-tooltip table-tooltip-dimensions\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{outputs.join(', ')}}</span>\n  </div>\n  <div class=\"row\">\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.PAYMENT_ID' | translate }}</span>\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[1] + sizes[2] + sizes[3] + 'px'\"\n          tooltip=\"{{transaction.payment_id}}\" placement=\"top\" tooltipClass=\"table-tooltip comment-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">\n      {{transaction.payment_id}}\n    </span>\n  </div>\n  <div class=\"row\">\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.COMMENT' | translate }}</span>\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[1] + sizes[2] + sizes[3] + 'px'\"\n          tooltip=\"{{transaction.comment}}\" placement=\"top\" tooltipClass=\"table-tooltip comment-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\"\n          (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, transaction.comment)\">\n      {{transaction.comment}}\n    </span>\n  </div>\n</div>\n"

/***/ }),

/***/ "./src/app/_helpers/directives/transaction-details/transaction-details.component.scss":
/*!********************************************************************************************!*\
  !*** ./src/app/_helpers/directives/transaction-details/transaction-details.component.scss ***!
  \********************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: absolute;\n  top: 0;\n  left: 0;\n  width: 100%; }\n\n.table {\n  border-top: 0.2rem solid #ebebeb;\n  margin: 0 3rem;\n  padding: 0.5rem 0; }\n\n.table .row {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-pack: start;\n            justify-content: flex-start;\n    -webkit-box-align: center;\n            align-items: center;\n    border-top: none;\n    line-height: 3rem;\n    margin: 0 -3rem;\n    width: 100%;\n    height: 3rem; }\n\n.table .row .cell {\n      flex-shrink: 0;\n      -webkit-box-flex: 0;\n              flex-grow: 0;\n      padding: 0 1rem;\n      overflow: hidden;\n      text-overflow: ellipsis; }\n\n.table .row .cell:first-child {\n        padding-left: 3rem; }\n\n.table .row .cell:last-child {\n        padding-right: 3rem; }\n\n.table .row .cell.key-value {\n        cursor: pointer; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL19oZWxwZXJzL2RpcmVjdGl2ZXMvdHJhbnNhY3Rpb24tZGV0YWlscy90cmFuc2FjdGlvbi1kZXRhaWxzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCO0VBQ2xCLE1BQU07RUFDTixPQUFPO0VBQ1AsV0FBVyxFQUFBOztBQUdiO0VBQ0UsZ0NBQWdDO0VBQ2hDLGNBQWM7RUFDZCxpQkFBaUIsRUFBQTs7QUFIbkI7SUFNSSxvQkFBYTtJQUFiLGFBQWE7SUFDYix1QkFBMkI7WUFBM0IsMkJBQTJCO0lBQzNCLHlCQUFtQjtZQUFuQixtQkFBbUI7SUFDbkIsZ0JBQWdCO0lBQ2hCLGlCQUFpQjtJQUNqQixlQUFlO0lBQ2YsV0FBVztJQUNYLFlBQVksRUFBQTs7QUFiaEI7TUFnQk0sY0FBYztNQUNkLG1CQUFZO2NBQVosWUFBWTtNQUNaLGVBQWU7TUFDZixnQkFBZ0I7TUFDaEIsdUJBQXVCLEVBQUE7O0FBcEI3QjtRQXVCUSxrQkFBa0IsRUFBQTs7QUF2QjFCO1FBMkJRLG1CQUFtQixFQUFBOztBQTNCM0I7UUErQlEsZUFBZSxFQUFBIiwiZmlsZSI6InNyYy9hcHAvX2hlbHBlcnMvZGlyZWN0aXZlcy90cmFuc2FjdGlvbi1kZXRhaWxzL3RyYW5zYWN0aW9uLWRldGFpbHMuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XG4gIHBvc2l0aW9uOiBhYnNvbHV0ZTtcbiAgdG9wOiAwO1xuICBsZWZ0OiAwO1xuICB3aWR0aDogMTAwJTtcbn1cblxuLnRhYmxlIHtcbiAgYm9yZGVyLXRvcDogMC4ycmVtIHNvbGlkICNlYmViZWI7XG4gIG1hcmdpbjogMCAzcmVtO1xuICBwYWRkaW5nOiAwLjVyZW0gMDtcblxuICAucm93IHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIGp1c3RpZnktY29udGVudDogZmxleC1zdGFydDtcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgIGJvcmRlci10b3A6IG5vbmU7XG4gICAgbGluZS1oZWlnaHQ6IDNyZW07XG4gICAgbWFyZ2luOiAwIC0zcmVtO1xuICAgIHdpZHRoOiAxMDAlO1xuICAgIGhlaWdodDogM3JlbTtcblxuICAgIC5jZWxsIHtcbiAgICAgIGZsZXgtc2hyaW5rOiAwO1xuICAgICAgZmxleC1ncm93OiAwO1xuICAgICAgcGFkZGluZzogMCAxcmVtO1xuICAgICAgb3ZlcmZsb3c6IGhpZGRlbjtcbiAgICAgIHRleHQtb3ZlcmZsb3c6IGVsbGlwc2lzO1xuXG4gICAgICAmOmZpcnN0LWNoaWxkIHtcbiAgICAgICAgcGFkZGluZy1sZWZ0OiAzcmVtO1xuICAgICAgfVxuXG4gICAgICAmOmxhc3QtY2hpbGQge1xuICAgICAgICBwYWRkaW5nLXJpZ2h0OiAzcmVtO1xuICAgICAgfVxuXG4gICAgICAmLmtleS12YWx1ZSB7XG4gICAgICAgIGN1cnNvcjogcG9pbnRlcjtcbiAgICAgIH1cbiAgICB9XG4gIH1cbn1cbiJdfQ== */"

/***/ }),

/***/ "./src/app/_helpers/directives/transaction-details/transaction-details.component.ts":
/*!******************************************************************************************!*\
  !*** ./src/app/_helpers/directives/transaction-details/transaction-details.component.ts ***!
  \******************************************************************************************/
/*! exports provided: TransactionDetailsComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "TransactionDetailsComponent", function() { return TransactionDetailsComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _models_transaction_model__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../../models/transaction.model */ "./src/app/_helpers/models/transaction.model.ts");
/* harmony import */ var _services_variables_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../../services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../../services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../../pipes/int-to-money.pipe */ "./src/app/_helpers/pipes/int-to-money.pipe.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};





var TransactionDetailsComponent = /** @class */ (function () {
    function TransactionDetailsComponent(variablesService, backendService, intToMoneyPipe) {
        this.variablesService = variablesService;
        this.backendService = backendService;
        this.intToMoneyPipe = intToMoneyPipe;
        this.inputs = [];
        this.outputs = [];
    }
    TransactionDetailsComponent.prototype.ngOnInit = function () {
        for (var input in this.transaction.td['spn']) {
            if (this.transaction.td['spn'].hasOwnProperty(input)) {
                this.inputs.push(this.intToMoneyPipe.transform(this.transaction.td['spn'][input]));
            }
        }
        for (var output in this.transaction.td['rcv']) {
            if (this.transaction.td['rcv'].hasOwnProperty(output)) {
                this.outputs.push(this.intToMoneyPipe.transform(this.transaction.td['rcv'][output]));
            }
        }
    };
    TransactionDetailsComponent.prototype.openInBrowser = function (tr) {
        this.backendService.openUrlInBrowser('explorer.zano.org/transaction/' + tr);
    };
    TransactionDetailsComponent.prototype.ngOnDestroy = function () { };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", _models_transaction_model__WEBPACK_IMPORTED_MODULE_1__["Transaction"])
    ], TransactionDetailsComponent.prototype, "transaction", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", Array)
    ], TransactionDetailsComponent.prototype, "sizes", void 0);
    TransactionDetailsComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-transaction-details',
            template: __webpack_require__(/*! ./transaction-details.component.html */ "./src/app/_helpers/directives/transaction-details/transaction-details.component.html"),
            styles: [__webpack_require__(/*! ./transaction-details.component.scss */ "./src/app/_helpers/directives/transaction-details/transaction-details.component.scss")]
        }),
        __metadata("design:paramtypes", [_services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"], _services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"], _pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_4__["IntToMoneyPipe"]])
    ], TransactionDetailsComponent);
    return TransactionDetailsComponent;
}());



/***/ }),

/***/ "./src/app/_helpers/models/transaction.model.ts":
/*!******************************************************!*\
  !*** ./src/app/_helpers/models/transaction.model.ts ***!
  \******************************************************/
/*! exports provided: Transaction */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "Transaction", function() { return Transaction; });
var Transaction = /** @class */ (function () {
    function Transaction() {
    }
    return Transaction;
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
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! bignumber.js */ "./node_modules/bignumber.js/bignumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_0___default = /*#__PURE__*/__webpack_require__.n(bignumber_js__WEBPACK_IMPORTED_MODULE_0__);

var Wallet = /** @class */ (function () {
    function Wallet(id, name, pass, path, address, balance, unlocked_balance, mined, tracking) {
        if (mined === void 0) { mined = 0; }
        if (tracking === void 0) { tracking = ''; }
        this.history = [];
        this.excluded_history = [];
        this.contracts = [];
        this.send_data = {
            address: null,
            amount: null,
            comment: null,
            mixin: null,
            fee: null,
            hide: null
        };
        this.wallet_id = id;
        this.name = name;
        this.pass = pass;
        this.path = path;
        this.address = address;
        this.balance = balance;
        this.unlocked_balance = unlocked_balance;
        this.mined_total = mined;
        this.tracking_hey = tracking;
        this.alias = {};
        this.staking = false;
        this.new_messages = 0;
        this.new_contracts = 0;
        this.history = [];
        this.excluded_history = [];
        this.progress = 0;
        this.loaded = false;
    }
    Wallet.prototype.getMoneyEquivalent = function (equivalent) {
        return this.balance.multipliedBy(equivalent).toFixed(0);
    };
    Wallet.prototype.havePass = function () {
        return (this.pass !== '' && this.pass !== null);
    };
    Wallet.prototype.isActive = function (id) {
        return this.wallet_id === id;
    };
    Wallet.prototype.prepareHistoryItem = function (item) {
        if (item.tx_type === 4) {
            item.sortFee = item.amount.plus(item.fee).negated();
            item.sortAmount = new bignumber_js__WEBPACK_IMPORTED_MODULE_0__["BigNumber"](0);
        }
        else if (item.tx_type === 3) {
            item.sortFee = new bignumber_js__WEBPACK_IMPORTED_MODULE_0__["BigNumber"](0);
        }
        else if ((item.hasOwnProperty('contract') && (item.contract[0].state === 3 || item.contract[0].state === 6 || item.contract[0].state === 601) && !item.contract[0].is_a)) {
            item.sortFee = item.fee.negated();
            item.sortAmount = item.amount;
        }
        else {
            if (!item.is_income) {
                item.sortFee = item.fee.negated();
                item.sortAmount = item.amount.negated();
            }
            else {
                item.sortAmount = item.amount;
            }
        }
        return item;
    };
    Wallet.prototype.prepareHistory = function (items) {
        for (var i = 0; i < items.length; i++) {
            if ((items[i].tx_type === 7 && items[i].is_income) || (items[i].tx_type === 11 && items[i].is_income) || (items[i].amount.eq(0) && items[i].fee.eq(0))) {
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
                    if (this.history.length && items[i].timestamp >= this.history[0].timestamp) {
                        this.history.unshift(this.prepareHistoryItem(items[i]));
                    }
                    else {
                        this.history.push(this.prepareHistoryItem(items[i]));
                    }
                }
            }
        }
    };
    Wallet.prototype.removeFromHistory = function (hash) {
        for (var i = 0; i < this.history.length; i++) {
            if (this.history[i].tx_hash === hash) {
                this.history.splice(i, 1);
                break;
            }
        }
    };
    Wallet.prototype.prepareContractsAfterOpen = function (items, exp_med_ts, height_app, viewedContracts, notViewedContracts) {
        var wallet = this;
        var _loop_1 = function (i) {
            var contract = items[i];
            var contractTransactionExist = false;
            if (wallet && wallet.history) {
                contractTransactionExist = wallet.history.some(function (elem) { return elem.contract && elem.contract.length && elem.contract[0].contract_id === contract.contract_id; });
            }
            if (!contractTransactionExist && wallet && wallet.excluded_history) {
                contractTransactionExist = wallet.excluded_history.some(function (elem) { return elem.contract && elem.contract.length && elem.contract[0].contract_id === contract.contract_id; });
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
            wallet.contracts.push(contract);
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
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};


var ContractStatusMessagesPipe = /** @class */ (function () {
    function ContractStatusMessagesPipe(translate) {
        this.translate = translate;
    }
    ContractStatusMessagesPipe.prototype.getStateSeller = function (stateNum) {
        var state = { part1: '', part2: '' };
        switch (stateNum) {
            case 1:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.NEW_CONTRACT');
                break;
            case 110:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.IGNORED');
                break;
            case 201:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.ACCEPTED');
                state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.WAIT');
                break;
            case 2:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.WAITING_BUYER');
                break;
            case 3:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.COMPLETED');
                break;
            case 4:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.NOT_RECEIVED');
                state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.NULLIFIED');
                break;
            case 5:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.PROPOSAL_CANCEL');
                break;
            case 601:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.BEING_CANCELLED');
                break;
            case 6:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.CANCELLED');
                break;
            case 130:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.IGNORED_CANCEL');
                break;
            case 140:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.EXPIRED');
                break;
        }
        return state.part1 + (state.part2.length ? '. ' + state.part2 : '');
    };
    ContractStatusMessagesPipe.prototype.getStateBuyer = function (stateNum) {
        var state = { part1: '', part2: '' };
        switch (stateNum) {
            case 1:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAITING');
                break;
            case 110:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.IGNORED');
                break;
            case 201:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.ACCEPTED');
                state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAIT');
                break;
            case 2:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.ACCEPTED');
                break;
            case 120:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAITING_SELLER');
                break;
            case 3:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.COMPLETED');
                break;
            case 4:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.NOT_RECEIVED');
                state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.NULLIFIED');
                break;
            case 5:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAITING_CANCEL');
                break;
            case 601:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.BEING_CANCELLED');
                break;
            case 6:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.CANCELLED');
                break;
            case 130:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.IGNORED_CANCEL');
                break;
            case 140:
                state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.EXPIRED');
                break;
        }
        return state.part1 + (state.part2.length ? '. ' + state.part2 : '');
    };
    ContractStatusMessagesPipe.prototype.transform = function (state, is_a) {
        if (is_a) {
            return this.getStateBuyer(state);
        }
        else {
            return this.getStateSeller(state);
        }
    };
    ContractStatusMessagesPipe = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Pipe"])({
            name: 'contractStatusMessages'
        }),
        __metadata("design:paramtypes", [_ngx_translate_core__WEBPACK_IMPORTED_MODULE_1__["TranslateService"]])
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
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};


var HistoryTypeMessagesPipe = /** @class */ (function () {
    function HistoryTypeMessagesPipe(translate) {
        this.translate = translate;
    }
    HistoryTypeMessagesPipe.prototype.transform = function (item, args) {
        if (item.tx_type === 0) {
            if (item.remote_addresses && item.remote_addresses[0]) {
                return item.remote_addresses[0];
            }
            else {
                if (item.is_income) {
                    return this.translate.instant('HISTORY.TYPE_MESSAGES.HIDDEN');
                }
                else {
                    return this.translate.instant('HISTORY.TYPE_MESSAGES.UNDEFINED');
                }
            }
        }
        else if (item.tx_type === 6 && item.height === 0) {
            return 'unknown';
        }
        else if (item.tx_type === 9) {
            if (item.hasOwnProperty('contract') && item.contract[0].is_a) {
                return this.translate.instant('HISTORY.TYPE_MESSAGES.COMPLETE_BUYER');
            }
            else {
                return this.translate.instant('HISTORY.TYPE_MESSAGES.COMPLETE_SELLER');
            }
        }
        else {
            switch (item.tx_type) {
                // case 0:
                //   return '';
                // case 1:
                //   return '';
                // case 2:
                //   return '';
                // case 3:
                //   return '';
                case 4:
                    return this.translate.instant('HISTORY.TYPE_MESSAGES.CREATE_ALIAS');
                case 5:
                    return this.translate.instant('HISTORY.TYPE_MESSAGES.UPDATE_ALIAS');
                case 6:
                    return (item.td['spn'] && item.td['spn'].length) ? this.translate.instant('HISTORY.TYPE_MESSAGES.POS_REWARD') : this.translate.instant('HISTORY.TYPE_MESSAGES.POW_REWARD');
                case 7:
                    return this.translate.instant('HISTORY.TYPE_MESSAGES.CREATE_CONTRACT');
                case 8:
                    return this.translate.instant('HISTORY.TYPE_MESSAGES.PLEDGE_CONTRACT');
                // case 9:
                //   return '';
                case 10:
                    return this.translate.instant('HISTORY.TYPE_MESSAGES.NULLIFY_CONTRACT');
                case 11:
                    return this.translate.instant('HISTORY.TYPE_MESSAGES.PROPOSAL_CANCEL_CONTRACT');
                case 12:
                    return this.translate.instant('HISTORY.TYPE_MESSAGES.CANCEL_CONTRACT');
            }
        }
        return this.translate.instant('HISTORY.TYPE_MESSAGES.UNDEFINED');
    };
    HistoryTypeMessagesPipe = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Pipe"])({
            name: 'historyTypeMessages'
        }),
        __metadata("design:paramtypes", [_ngx_translate_core__WEBPACK_IMPORTED_MODULE_1__["TranslateService"]])
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
/* harmony import */ var _services_variables_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! bignumber.js */ "./node_modules/bignumber.js/bignumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_2___default = /*#__PURE__*/__webpack_require__.n(bignumber_js__WEBPACK_IMPORTED_MODULE_2__);
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};



var IntToMoneyPipe = /** @class */ (function () {
    function IntToMoneyPipe(variablesService) {
        this.variablesService = variablesService;
    }
    IntToMoneyPipe.prototype.transform = function (value, args) {
        if (value === 0 || value === undefined) {
            return '0';
        }
        var maxFraction = this.variablesService.digits;
        if (args) {
            maxFraction = parseInt(args, 10);
        }
        var power = Math.pow(10, this.variablesService.digits);
        var str = (new bignumber_js__WEBPACK_IMPORTED_MODULE_2__["BigNumber"](value)).div(power).toFixed(maxFraction);
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
        }),
        __metadata("design:paramtypes", [_services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"]])
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
/* harmony import */ var _services_variables_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! bignumber.js */ "./node_modules/bignumber.js/bignumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_2___default = /*#__PURE__*/__webpack_require__.n(bignumber_js__WEBPACK_IMPORTED_MODULE_2__);
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};



var MoneyToIntPipe = /** @class */ (function () {
    function MoneyToIntPipe(variablesService) {
        this.variablesService = variablesService;
    }
    MoneyToIntPipe.prototype.transform = function (value, args) {
        var CURRENCY_DISPLAY_DECIMAL_POINT = this.variablesService.digits;
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
            result = (new bignumber_js__WEBPACK_IMPORTED_MODULE_2__["BigNumber"](am_str)).integerValue();
        }
        return result;
    };
    MoneyToIntPipe = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Pipe"])({
            name: 'moneyToInt'
        }),
        __metadata("design:paramtypes", [_services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"]])
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
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
/* harmony import */ var _variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ./variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _modal_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ./modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var _pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../pipes/money-to-int.pipe */ "./src/app/_helpers/pipes/money-to-int.pipe.ts");
/* harmony import */ var json_bignumber__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! json-bignumber */ "./node_modules/json-bignumber/src/JSONBigNumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! bignumber.js */ "./node_modules/bignumber.js/bignumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_7___default = /*#__PURE__*/__webpack_require__.n(bignumber_js__WEBPACK_IMPORTED_MODULE_7__);
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
    function BackendService(translate, variablesService, modalService, moneyToIntPipe) {
        this.translate = translate;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.moneyToIntPipe = moneyToIntPipe;
        this.backendLoaded = false;
    }
    BackendService_1 = BackendService;
    BackendService.bigNumberParser = function (key, val) {
        if (val.constructor.name === 'BigNumber' && ['balance', 'unlocked_balance', 'amount', 'fee', 'b_fee', 'to_pay', 'a_pledge', 'b_pledge', 'coast', 'a'].indexOf(key) === -1) {
            return val.toNumber();
        }
        if (key === 'rcv' || key === 'spn') {
            for (var i = 0; i < val.length; i++) {
                val[i] = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](val[i]);
            }
        }
        return val;
    };
    BackendService.Debug = function (type, message) {
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
                error_translate = 'ERRORS.NOT_ENOUGH_MONEY';
                break;
            case 'CORE_BUSY':
                if (command !== 'get_all_aliases') {
                    error_translate = 'ERRORS.CORE_BUSY';
                }
                break;
            case 'OVERFLOW':
                if (command !== 'get_all_aliases') {
                    error_translate = '';
                }
                break;
            case 'INTERNAL_ERROR:daemon is busy':
                error_translate = 'ERRORS.DAEMON_BUSY';
                break;
            case 'INTERNAL_ERROR:not enough money':
            case 'INTERNAL_ERROR:NOT_ENOUGH_MONEY':
                if (command === 'cancel_offer') {
                    error_translate = this.translate.instant('ERRORS.NO_MONEY_REMOVE_OFFER', {
                        'fee': this.variablesService.default_fee,
                        'currency': this.variablesService.defaultCurrency
                    });
                }
                else {
                    error_translate = 'ERRORS.NO_MONEY';
                }
                break;
            case 'INTERNAL_ERROR:not enough outputs to mix':
                error_translate = 'ERRORS.NOT_ENOUGH_OUTPUTS_TO_MIX';
                break;
            case 'INTERNAL_ERROR:transaction is too big':
                error_translate = 'ERRORS.TRANSACTION_IS_TO_BIG';
                break;
            case 'INTERNAL_ERROR:Transfer attempt while daemon offline':
                error_translate = 'ERRORS.TRANSFER_ATTEMPT';
                break;
            case 'ACCESS_DENIED':
                error_translate = 'ERRORS.ACCESS_DENIED';
                break;
            case 'INTERNAL_ERROR:transaction was rejected by daemon':
                // if (command === 'request_alias_registration') {
                // error_translate = 'INFORMER.ALIAS_IN_REGISTER';
                // } else {
                error_translate = 'ERRORS.TRANSACTION_ERROR';
                // }
                break;
            case 'INTERNAL_ERROR':
                error_translate = 'ERRORS.TRANSACTION_ERROR';
                break;
            case 'BAD_ARG':
                error_translate = 'ERRORS.BAD_ARG';
                break;
            case 'WALLET_WRONG_ID':
                error_translate = 'ERRORS.WALLET_WRONG_ID';
                break;
            case 'WRONG_PASSWORD':
            case 'WRONG_PASSWORD:invalid password':
                params = JSON.parse(params);
                if (!params.testEmpty) {
                    error_translate = 'ERRORS.WRONG_PASSWORD';
                }
                break;
            case 'FILE_RESTORED':
                if (command === 'open_wallet') {
                    error_translate = 'ERRORS.FILE_RESTORED';
                }
                break;
            case 'FILE_NOT_FOUND':
                if (command !== 'open_wallet' && command !== 'get_alias_info_by_name' && command !== 'get_alias_info_by_address') {
                    error_translate = this.translate.instant('ERRORS.FILE_NOT_FOUND');
                    params = JSON.parse(params);
                    if (params.path) {
                        error_translate += ': ' + params.path;
                    }
                }
                break;
            case 'NOT_FOUND':
                if (command !== 'open_wallet' && command !== 'get_alias_info_by_name' && command !== 'get_alias_info_by_address') {
                    error_translate = this.translate.instant('ERRORS.FILE_NOT_FOUND');
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
                error_translate = 'ERRORS.FILE_EXIST';
                break;
            default:
                error_translate = error;
        }
        if (error.indexOf('FAIL:failed to save file') > -1) {
            error_translate = 'ERRORS.FILE_NOT_SAVED';
        }
        if (error.indexOf('FAILED:failed to open binary wallet file for saving') > -1 && command === 'generate_wallet') {
            error_translate = '';
        }
        if (error_translate !== '') {
            this.modalService.prepareModal('error', error_translate);
        }
    };
    BackendService.prototype.commandDebug = function (command, params, result) {
        BackendService_1.Debug(2, '----------------- ' + command + ' -----------------');
        var debug = {
            _send_params: params,
            _result: result
        };
        BackendService_1.Debug(2, debug);
        try {
            BackendService_1.Debug(2, json_bignumber__WEBPACK_IMPORTED_MODULE_6__["default"].parse(result, BackendService_1.bigNumberParser));
        }
        catch (e) {
            BackendService_1.Debug(2, { response_data: result, error_code: 'OK' });
        }
    };
    BackendService.prototype.backendCallback = function (resultStr, params, callback, command) {
        var Result = resultStr;
        if (command !== 'get_clipboard') {
            if (!resultStr || resultStr === '') {
                Result = {};
            }
            else {
                try {
                    Result = json_bignumber__WEBPACK_IMPORTED_MODULE_6__["default"].parse(resultStr, BackendService_1.bigNumberParser);
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
            BackendService_1.Debug(1, 'API error for command: "' + command + '". Error code: ' + Result.error_code);
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
                BackendService_1.Debug(0, 'Run Command Error! Command "' + command + '" don\'t found in backendObject');
            }
            else {
                var that_1 = this;
                params = (typeof params === 'string') ? params : json_bignumber__WEBPACK_IMPORTED_MODULE_6__["default"].stringify(params);
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
                callback(json_bignumber__WEBPACK_IMPORTED_MODULE_6__["default"].parse(str, BackendService_1.bigNumberParser));
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
        var _this = this;
        if (this.variablesService.wallets.length) {
            this.variablesService.settings.wallets = [];
            this.variablesService.wallets.forEach(function (wallet) {
                _this.variablesService.settings.wallets.push({ name: wallet.name, path: wallet.path });
            });
        }
        this.runCommand('store_app_data', this.variablesService.settings, callback);
    };
    BackendService.prototype.getSecureAppData = function (pass, callback) {
        this.runCommand('get_secure_app_data', pass, callback);
    };
    BackendService.prototype.storeSecureAppData = function (callback) {
        var _this = this;
        var wallets = [];
        this.variablesService.wallets.forEach(function (wallet) {
            wallets.push({ name: wallet.name, pass: wallet.pass, path: wallet.path, staking: wallet.staking });
        });
        this.backendObject['store_secure_app_data'](JSON.stringify(wallets), this.variablesService.appPass, function (dataStore) {
            _this.backendCallback(dataStore, {}, callback, 'store_secure_app_data');
        });
    };
    BackendService.prototype.dropSecureAppData = function (callback) {
        var _this = this;
        this.backendObject['drop_secure_app_data'](function (dataStore) {
            _this.backendCallback(dataStore, {}, callback, 'drop_secure_app_data');
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
        this.runCommand('close_wallet', { wallet_id: +wallet_id }, callback);
    };
    BackendService.prototype.getSmartWalletInfo = function (wallet_id, callback) {
        this.runCommand('get_smart_wallet_info', { wallet_id: +wallet_id }, callback);
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
    BackendService.prototype.sendMoney = function (from_wallet_id, to_address, amount, fee, mixin, comment, hide, callback) {
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
            push_payer: !hide
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
                a_pledge: this.moneyToIntPipe.transform(a_pledge),
                b_pledge: this.moneyToIntPipe.transform(b_pledge)
            },
            payment_id: payment_id,
            expiration_period: parseInt(time, 10) * 60 * 60,
            fee: this.variablesService.default_fee_big,
            b_fee: this.variablesService.default_fee_big
        };
        BackendService_1.Debug(1, params);
        this.runCommand('create_proposal', params, callback);
    };
    BackendService.prototype.getContracts = function (wallet_id, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10)
        };
        BackendService_1.Debug(1, params);
        this.runCommand('get_contracts', params, callback);
    };
    BackendService.prototype.acceptProposal = function (wallet_id, contract_id, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10),
            contract_id: contract_id
        };
        BackendService_1.Debug(1, params);
        this.runCommand('accept_proposal', params, callback);
    };
    BackendService.prototype.releaseProposal = function (wallet_id, contract_id, release_type, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10),
            contract_id: contract_id,
            release_type: release_type // "normal" or "burn"
        };
        BackendService_1.Debug(1, params);
        this.runCommand('release_contract', params, callback);
    };
    BackendService.prototype.requestCancelContract = function (wallet_id, contract_id, time, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10),
            contract_id: contract_id,
            fee: this.variablesService.default_fee_big,
            expiration_period: parseInt(time, 10) * 60 * 60
        };
        BackendService_1.Debug(1, params);
        this.runCommand('request_cancel_contract', params, callback);
    };
    BackendService.prototype.acceptCancelContract = function (wallet_id, contract_id, callback) {
        var params = {
            wallet_id: parseInt(wallet_id, 10),
            contract_id: contract_id
        };
        BackendService_1.Debug(1, params);
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
    BackendService.prototype.openUrlInBrowser = function (url, callback) {
        this.runCommand('open_url_in_browser', url, callback);
    };
    BackendService.prototype.start_backend = function (node, host, port, callback) {
        var params = {
            configure_for_remote_node: node,
            remote_node_host: host,
            remote_node_port: parseInt(port, 10)
        };
        this.runCommand('start_backend', params, callback);
    };
    BackendService.prototype.getDefaultFee = function (callback) {
        this.runCommand('get_default_fee', {}, callback);
    };
    BackendService.prototype.setBackendLocalization = function (stringsArray, title, callback) {
        var params = {
            strings: stringsArray,
            language_title: title
        };
        this.runCommand('set_localization_strings', params, callback);
    };
    BackendService.prototype.registerAlias = function (wallet_id, alias, address, fee, comment, reward, callback) {
        var params = {
            wallet_id: wallet_id,
            alias: {
                alias: alias,
                address: address,
                tracking_key: '',
                comment: comment
            },
            fee: this.moneyToIntPipe.transform(fee),
            reward: this.moneyToIntPipe.transform(reward)
        };
        this.runCommand('request_alias_registration', params, callback);
    };
    BackendService.prototype.updateAlias = function (wallet_id, alias, fee, callback) {
        var params = {
            wallet_id: wallet_id,
            alias: {
                alias: alias.name.replace('@', ''),
                address: alias.address,
                tracking_key: '',
                comment: alias.comment
            },
            fee: this.moneyToIntPipe.transform(fee)
        };
        this.runCommand('request_alias_update', params, callback);
    };
    BackendService.prototype.getAllAliases = function (callback) {
        this.runCommand('get_all_aliases', {}, callback);
    };
    BackendService.prototype.getAliasByName = function (value, callback) {
        return this.runCommand('get_alias_info_by_name', value, callback);
    };
    BackendService.prototype.getAliasByAddress = function (value, callback) {
        return this.runCommand('get_alias_info_by_address', value, callback);
    };
    BackendService.prototype.getAliasCoast = function (alias, callback) {
        this.runCommand('get_alias_coast', { v: alias }, callback);
    };
    BackendService.prototype.getWalletAlias = function (address) {
        var _this = this;
        if (address !== null && this.variablesService.daemon_state === 2) {
            if (this.variablesService.aliasesChecked[address] == null) {
                this.variablesService.aliasesChecked[address] = {};
                if (this.variablesService.aliases.length) {
                    for (var i = 0, length_1 = this.variablesService.aliases.length; i < length_1; i++) {
                        if (i in this.variablesService.aliases && this.variablesService.aliases[i]['address'] === address) {
                            this.variablesService.aliasesChecked[address]['name'] = this.variablesService.aliases[i].name;
                            this.variablesService.aliasesChecked[address]['address'] = this.variablesService.aliases[i].address;
                            this.variablesService.aliasesChecked[address]['comment'] = this.variablesService.aliases[i].comment;
                            return this.variablesService.aliasesChecked[address];
                        }
                    }
                }
                this.getAliasByAddress(address, function (status, data) {
                    if (status) {
                        _this.variablesService.aliasesChecked[data.address]['name'] = '@' + data.alias;
                        _this.variablesService.aliasesChecked[data.address]['address'] = data.address;
                        _this.variablesService.aliasesChecked[data.address]['comment'] = data.comment;
                    }
                });
            }
            return this.variablesService.aliasesChecked[address];
        }
        return {};
    };
    BackendService.prototype.getPoolInfo = function (callback) {
        this.runCommand('get_tx_pool_info', {}, callback);
    };
    BackendService.prototype.getVersion = function (callback) {
        this.runCommand('get_version', {}, function (status, version) {
            callback(version);
        });
    };
    BackendService.prototype.setLogLevel = function (level) {
        return this.runCommand('set_log_level', { v: level });
    };
    var BackendService_1;
    BackendService = BackendService_1 = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Injectable"])(),
        __metadata("design:paramtypes", [_ngx_translate_core__WEBPACK_IMPORTED_MODULE_2__["TranslateService"],
            _variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _modal_service__WEBPACK_IMPORTED_MODULE_4__["ModalService"],
            _pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_5__["MoneyToIntPipe"]])
    ], BackendService);
    return BackendService;
}());

/*

      toggleAutoStart: function (value) {
        return this.runCommand('toggle_autostart', asVal(value));
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

      resetWalletPass: function (wallet_id, pass, callback) {
        this.runCommand('reset_wallet_password', {wallet_id: wallet_id, pass: pass}, callback);
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

*/


/***/ }),

/***/ "./src/app/_helpers/services/modal.service.ts":
/*!****************************************************!*\
  !*** ./src/app/_helpers/services/modal.service.ts ***!
  \****************************************************/
/*! exports provided: ModalService */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ModalService", function() { return ModalService; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
/* harmony import */ var _directives_modal_container_modal_container_component__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../directives/modal-container/modal-container.component */ "./src/app/_helpers/directives/modal-container/modal-container.component.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};



var ModalService = /** @class */ (function () {
    function ModalService(componentFactoryResolver, appRef, injector, ngZone, translate) {
        this.componentFactoryResolver = componentFactoryResolver;
        this.appRef = appRef;
        this.injector = injector;
        this.ngZone = ngZone;
        this.translate = translate;
        this.components = [];
    }
    ModalService.prototype.prepareModal = function (type, message) {
        var _this = this;
        var length = this.components.push(this.componentFactoryResolver.resolveComponentFactory(_directives_modal_container_modal_container_component__WEBPACK_IMPORTED_MODULE_2__["ModalContainerComponent"]).create(this.injector));
        this.components[length - 1].instance['type'] = type;
        this.components[length - 1].instance['message'] = message.length ? this.translate.instant(message) : '';
        this.components[length - 1].instance['close'].subscribe(function () {
            _this.removeModal(length - 1);
        });
        this.ngZone.run(function () {
            _this.appendModal(length - 1);
        });
    };
    ModalService.prototype.appendModal = function (index) {
        this.appRef.attachView(this.components[index].hostView);
        var domElem = this.components[index].hostView.rootNodes[0];
        document.body.appendChild(domElem);
    };
    ModalService.prototype.removeModal = function (index) {
        if (this.components[index]) {
            this.appRef.detachView(this.components[index].hostView);
            this.components[index].destroy();
            this.components.splice(index, 1);
        }
        else {
            var last = this.components.length - 1;
            this.appRef.detachView(this.components[last].hostView);
            this.components[last].destroy();
            this.components.splice(last, 1);
        }
    };
    ModalService = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Injectable"])(),
        __metadata("design:paramtypes", [_angular_core__WEBPACK_IMPORTED_MODULE_0__["ComponentFactoryResolver"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["ApplicationRef"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["Injector"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_1__["TranslateService"]])
    ], ModalService);
    return ModalService;
}());



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
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! bignumber.js */ "./node_modules/bignumber.js/bignumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_5___default = /*#__PURE__*/__webpack_require__.n(bignumber_js__WEBPACK_IMPORTED_MODULE_5__);
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
        this.digits = 12;
        this.appPass = '';
        this.appLogin = false;
        this.moneyEquivalent = 0;
        this.defaultTheme = 'dark';
        this.defaultCurrency = 'ZANO';
        this.exp_med_ts = 0;
        this.net_time_delta_median = 0;
        this.height_app = 0;
        this.last_build_available = '';
        this.last_build_displaymode = 0;
        this.daemon_state = 3;
        this.sync = {
            progress_value: 0,
            progress_value_text: '0'
        };
        this.default_fee = '0.010000000000';
        this.default_fee_big = new bignumber_js__WEBPACK_IMPORTED_MODULE_5__["BigNumber"]('10000000000');
        this.settings = {
            appLockTime: 15,
            appLog: 0,
            theme: '',
            scale: 10,
            language: 'en',
            default_path: '/',
            viewedContracts: [],
            notViewedContracts: [],
            wallets: []
        };
        this.wallets = [];
        this.aliases = [];
        this.aliasesChecked = {};
        this.enableAliasSearch = false;
        this.maxWalletNameLength = 25;
        this.maxCommentLength = 255;
        this.getExpMedTsEvent = new rxjs__WEBPACK_IMPORTED_MODULE_1__["BehaviorSubject"](null);
        this.getHeightAppEvent = new rxjs__WEBPACK_IMPORTED_MODULE_1__["BehaviorSubject"](null);
        this.getRefreshStackingEvent = new rxjs__WEBPACK_IMPORTED_MODULE_1__["BehaviorSubject"](null);
        this.getAliasChangedEvent = new rxjs__WEBPACK_IMPORTED_MODULE_1__["BehaviorSubject"](null);
        this.idle = new idlejs_dist__WEBPACK_IMPORTED_MODULE_2__["Idle"]()
            .whenNotInteractive()
            .do(function () {
            _this.ngZone.run(function () {
                _this.idle.stop();
                _this.appPass = '';
                _this.appLogin = false;
                _this.router.navigate(['/login'], { queryParams: { type: 'auth' } });
            });
        });
    }
    VariablesService.prototype.setExpMedTs = function (timestamp) {
        if (timestamp !== this.exp_med_ts) {
            this.exp_med_ts = timestamp;
            this.getExpMedTsEvent.next(timestamp);
        }
    };
    VariablesService.prototype.setHeightApp = function (height) {
        if (height !== this.height_app) {
            this.height_app = height;
            this.getHeightAppEvent.next(height);
        }
    };
    VariablesService.prototype.setRefreshStacking = function (wallet_id) {
        this.getHeightAppEvent.next(wallet_id);
    };
    VariablesService.prototype.changeAliases = function () {
        this.getAliasChangedEvent.next(true);
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
        this.idle.within(this.settings.appLockTime).start();
    };
    VariablesService.prototype.stopCountdown = function () {
        this.idle.stop();
    };
    VariablesService.prototype.restartCountdown = function () {
        this.idle.within(this.settings.appLockTime).restart();
    };
    VariablesService.prototype.onContextMenu = function ($event) {
        $event.target['contextSelectionStart'] = $event.target['selectionStart'];
        $event.target['contextSelectionEnd'] = $event.target['selectionEnd'];
        if ($event.target && ($event.target['nodeName'].toUpperCase() === 'TEXTAREA' || $event.target['nodeName'].toUpperCase() === 'INPUT') && !$event.target['readOnly']) {
            this.contextMenuService.show.next({
                contextMenu: this.allContextMenu,
                event: $event,
                item: $event.target,
            });
            $event.preventDefault();
            $event.stopPropagation();
        }
    };
    VariablesService.prototype.onContextMenuOnlyCopy = function ($event, copyText) {
        this.contextMenuService.show.next({
            contextMenu: this.onlyCopyContextMenu,
            event: $event,
            item: copyText
        });
        $event.preventDefault();
        $event.stopPropagation();
    };
    VariablesService.prototype.onContextMenuPasteSelect = function ($event) {
        $event.target['contextSelectionStart'] = $event.target['selectionStart'];
        $event.target['contextSelectionEnd'] = $event.target['selectionEnd'];
        console.warn($event.target);
        console.warn($event.target['disabled']);
        if ($event.target && ($event.target['nodeName'].toUpperCase() === 'TEXTAREA' || $event.target['nodeName'].toUpperCase() === 'INPUT') && !$event.target['readOnly']) {
            this.contextMenuService.show.next({
                contextMenu: this.pasteSelectContextMenu,
                event: $event,
                item: $event.target,
            });
            $event.preventDefault();
            $event.stopPropagation();
        }
    };
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
/* harmony import */ var _assign_alias_assign_alias_component__WEBPACK_IMPORTED_MODULE_19__ = __webpack_require__(/*! ./assign-alias/assign-alias.component */ "./src/app/assign-alias/assign-alias.component.ts");
/* harmony import */ var _edit_alias_edit_alias_component__WEBPACK_IMPORTED_MODULE_20__ = __webpack_require__(/*! ./edit-alias/edit-alias.component */ "./src/app/edit-alias/edit-alias.component.ts");
/* harmony import */ var _transfer_alias_transfer_alias_component__WEBPACK_IMPORTED_MODULE_21__ = __webpack_require__(/*! ./transfer-alias/transfer-alias.component */ "./src/app/transfer-alias/transfer-alias.component.ts");
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
                redirectTo: 'history',
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
        path: 'assign-alias',
        component: _assign_alias_assign_alias_component__WEBPACK_IMPORTED_MODULE_19__["AssignAliasComponent"]
    },
    {
        path: 'edit-alias',
        component: _edit_alias_edit_alias_component__WEBPACK_IMPORTED_MODULE_20__["EditAliasComponent"]
    },
    {
        path: 'transfer-alias',
        component: _transfer_alias_transfer_alias_component__WEBPACK_IMPORTED_MODULE_21__["TransferAliasComponent"]
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

module.exports = "<app-sidebar *ngIf=\"variablesService.appLogin\"></app-sidebar>\n\n<div class=\"app-content scrolled-content\">\n  <router-outlet *ngIf=\"[0, 1, 2].indexOf(variablesService.daemon_state) !== -1\"></router-outlet>\n  <div class=\"preloader\" *ngIf=\"[3, 4, 5].indexOf(variablesService.daemon_state) !== -1\">\n    <span *ngIf=\"variablesService.daemon_state === 3\">{{ 'SIDEBAR.SYNCHRONIZATION.LOADING' | translate }}</span>\n    <span *ngIf=\"variablesService.daemon_state === 4\">{{ 'SIDEBAR.SYNCHRONIZATION.ERROR' | translate }}</span>\n    <span *ngIf=\"variablesService.daemon_state === 5\">{{ 'SIDEBAR.SYNCHRONIZATION.COMPLETE' | translate }}</span>\n    <span class=\"loading-bar\"></span>\n  </div>\n</div>\n\n<context-menu #allContextMenu>\n  <ng-template contextMenuItem (execute)=\"contextMenuCopy($event.item)\">{{ 'CONTEXT_MENU.COPY' | translate }}</ng-template>\n  <ng-template contextMenuItem (execute)=\"contextMenuPaste($event.item)\">{{ 'CONTEXT_MENU.PASTE' | translate }}</ng-template>\n  <ng-template contextMenuItem (execute)=\"contextMenuSelect($event.item)\">{{ 'CONTEXT_MENU.SELECT' | translate }}</ng-template>\n</context-menu>\n\n<context-menu #onlyCopyContextMenu>\n  <ng-template contextMenuItem (execute)=\"contextMenuOnlyCopy($event.item)\">{{ 'CONTEXT_MENU.COPY' | translate }}</ng-template>\n</context-menu>\n\n<context-menu #pasteSelectContextMenu>\n  <ng-template contextMenuItem (execute)=\"contextMenuPaste($event.item)\">{{ 'CONTEXT_MENU.PASTE' | translate }}</ng-template>\n  <ng-template contextMenuItem (execute)=\"contextMenuSelect($event.item)\">{{ 'CONTEXT_MENU.SELECT' | translate }}</ng-template>\n</context-menu>\n\n\n<app-open-wallet-modal *ngIf=\"needOpenWallets.length\" [wallets]=\"needOpenWallets\"></app-open-wallet-modal>\n"

/***/ }),

/***/ "./src/app/app.component.scss":
/*!************************************!*\
  !*** ./src/app/app.component.scss ***!
  \************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "/*\n* Implementation of themes\n*/\n.app-content {\n  display: -webkit-box;\n  display: flex;\n  overflow-x: overlay;\n  overflow-y: hidden;\n  width: 100%; }\n.app-content .preloader {\n    align-self: center;\n    color: #fff;\n    font-size: 2rem;\n    margin: 0 auto;\n    text-align: center;\n    width: 50%; }\n.app-content .preloader .loading-bar {\n      display: block;\n      -webkit-animation: move 5s linear infinite;\n              animation: move 5s linear infinite;\n      background-image: -webkit-gradient(linear, 0 0, 100% 100%, color-stop(0.125, rgba(0, 0, 0, 0.15)), color-stop(0.125, transparent), color-stop(0.25, transparent), color-stop(0.25, rgba(0, 0, 0, 0.1)), color-stop(0.375, rgba(0, 0, 0, 0.1)), color-stop(0.375, transparent), color-stop(0.5, transparent), color-stop(0.5, rgba(0, 0, 0, 0.15)), color-stop(0.625, rgba(0, 0, 0, 0.15)), color-stop(0.625, transparent), color-stop(0.75, transparent), color-stop(0.75, rgba(0, 0, 0, 0.1)), color-stop(0.875, rgba(0, 0, 0, 0.1)), color-stop(0.875, transparent), to(transparent)), -webkit-gradient(linear, 0 100%, 100% 0, color-stop(0.125, rgba(0, 0, 0, 0.3)), color-stop(0.125, transparent), color-stop(0.25, transparent), color-stop(0.25, rgba(0, 0, 0, 0.25)), color-stop(0.375, rgba(0, 0, 0, 0.25)), color-stop(0.375, transparent), color-stop(0.5, transparent), color-stop(0.5, rgba(0, 0, 0, 0.3)), color-stop(0.625, rgba(0, 0, 0, 0.3)), color-stop(0.625, transparent), color-stop(0.75, transparent), color-stop(0.75, rgba(0, 0, 0, 0.25)), color-stop(0.875, rgba(0, 0, 0, 0.25)), color-stop(0.875, transparent), to(transparent));\n      background-size: 10rem 10rem;\n      margin-top: 2rem;\n      width: 100%;\n      height: 1rem; }\n@-webkit-keyframes move {\n  0% {\n    background-position: 100% -10rem; }\n  100% {\n    background-position: 100% 10rem; } }\n@keyframes move {\n  0% {\n    background-position: 100% -10rem; }\n  100% {\n    background-position: 100% 10rem; } }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXNzZXRzL3Njc3MvYmFzZS9fbWl4aW5zLnNjc3MiLCJzcmMvYXBwL2FwcC5jb21wb25lbnQuc2NzcyIsIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL2FwcC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUE4RUE7O0NDNUVDO0FDQUQ7RUFDRSxvQkFBYTtFQUFiLGFBQWE7RUFDYixtQkFBbUI7RUFDbkIsa0JBQWtCO0VBQ2xCLFdBQVcsRUFBQTtBQUpiO0lBT0ksa0JBQWtCO0lBQ2xCLFdBQVc7SUFDWCxlQUFlO0lBQ2YsY0FBYztJQUNkLGtCQUFrQjtJQUNsQixVQUFVLEVBQUE7QUFaZDtNQWVNLGNBQWM7TUFDZCwwQ0FBa0M7Y0FBbEMsa0NBQWtDO01BQ2xDLCtsQ0FzQkc7TUFDSCw0QkFBNEI7TUFDNUIsZ0JBQWdCO01BQ2hCLFdBQVc7TUFDWCxZQUFZLEVBQUE7QUFJaEI7RUFDRTtJQUNFLGdDQUFnQyxFQUFBO0VBRWxDO0lBQ0UsK0JBQStCLEVBQUEsRUFBQTtBQUxuQztFQUNFO0lBQ0UsZ0NBQWdDLEVBQUE7RUFFbEM7SUFDRSwrQkFBK0IsRUFBQSxFQUFBIiwiZmlsZSI6InNyYy9hcHAvYXBwLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiQG1peGluIHRleHQtdHJ1bmNhdGUge1xuICBvdmVyZmxvdzogaGlkZGVuO1xuICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcbiAgd2hpdGUtc3BhY2U6IG5vd3JhcDtcbn1cbkBtaXhpbiB0ZXh0V3JhcCB7XG4gIHdoaXRlLXNwYWNlOiBub3JtYWw7XG4gIG92ZXJmbG93LXdyYXA6IGJyZWFrLXdvcmQ7XG4gIHdvcmQtd3JhcDogYnJlYWstd29yZDtcbiAgd29yZC1icmVhazogYnJlYWstYWxsO1xuICBsaW5lLWJyZWFrOiBzdHJpY3Q7XG4gIC13ZWJraXQtaHlwaGVuczogYXV0bztcbiAgLW1zLWh5cGhlbnM6IGF1dG87XG4gIGh5cGhlbnM6IGF1dG87XG59XG5AbWl4aW4gY292ZXJCb3gge1xuXHRwb3NpdGlvbjogYWJzb2x1dGU7XG5cdHRvcDogMDtcblx0bGVmdDogMDtcblx0d2lkdGg6IDEwMCU7XG5cdGhlaWdodDogMTAwJTtcbn1cbkBtaXhpbiBhYnMgKCR0b3A6IGF1dG8sICRyaWdodDogYXV0bywgJGJvdHRvbTogYXV0bywgJGxlZnQ6IGF1dG8pIHtcbiAgdG9wOiAkdG9wO1xuICByaWdodDogJHJpZ2h0O1xuICBib3R0b206ICRib3R0b207XG4gIGxlZnQ6ICRsZWZ0O1xuICBwb3NpdGlvbjogYWJzb2x1dGU7XG59XG5AbWl4aW4gY292ZXJJbWcge1xuXHRiYWNrZ3JvdW5kLXJlcGVhdDogbm8tcmVwZWF0O1xuXHQtd2Via2l0LWJhY2tncm91bmQtc2l6ZTogY292ZXI7XG5cdC1vLWJhY2tncm91bmQtc2l6ZTogY292ZXI7XG5cdGJhY2tncm91bmQtc2l6ZTogY292ZXI7XG5cdGJhY2tncm91bmQtcG9zaXRpb246IDUwJSA1MCU7XG59XG5AbWl4aW4gdmFsaW5nQm94IHtcblx0cG9zaXRpb246IGFic29sdXRlO1xuXHR0b3A6ICA1MCU7XG5cdGxlZnQ6IDUwJTtcblx0dHJhbnNmb3JtOiB0cmFuc2xhdGUoLTUwJSwgLTUwJSk7XG59XG5AbWl4aW4gdW5TZWxlY3Qge1xuXHQtd2Via2l0LXRvdWNoLWNvbGxvdXQ6IG5vbmU7XG4gIC13ZWJraXQtdXNlci1zZWxlY3Q6IG5vbmU7XG4gIC1raHRtbC11c2VyLXNlbGVjdDogbm9uZTtcbiAgLW1vei11c2VyLXNlbGVjdDogbm9uZTtcbiAgLW1zLXVzZXItc2VsZWN0OiBub25lO1xuICB1c2VyLXNlbGVjdDogbm9uZTtcbn1cbkBtaXhpbiBtYXgxMTk5IHsgLy8gbWFrZXQgMTE3MVxuICBAbWVkaWEgKG1heC13aWR0aDogMTE5OXB4KSB7IEBjb250ZW50OyB9XG59XG5AbWl4aW4gbWF4MTE3MCB7IC8vIG1ha2V0cyA5OTJcbiAgQG1lZGlhIChtYXgtd2lkdGg6IDExNzBweCkgeyBAY29udGVudDsgfVxufVxuQG1peGluIG1heDk5MSB7IC8vIG1ha2V0cyA3NjJcbiAgQG1lZGlhIChtYXgtd2lkdGg6IDk5MXB4KSB7IEBjb250ZW50OyB9XG59XG5AbWl4aW4gbWF4NzYxIHsgLy8gbWFrZXRzIDU3NlxuICBAbWVkaWEgKG1heC13aWR0aDogNzYxcHgpIHsgQGNvbnRlbnQ7IH1cbn1cbkBtaXhpbiBtYXg1NzUgeyAvLyBtYWtldHMgNDAwXG4gIEBtZWRpYSAobWF4LXdpZHRoOiA1NzVweCkgeyBAY29udGVudDsgfVxufVxuQG1peGluIG1vYmlsZSB7XG4gIEBtZWRpYSAobWF4LXdpZHRoOiAzOTlweCkgeyBAY29udGVudDsgfVxufVxuQG1peGluIGljb0NlbnRlciB7XG4gICAgYmFja2dyb3VuZC1yZXBlYXQ6IG5vLXJlcGVhdDtcbiAgICBiYWNrZ3JvdW5kLXBvc2l0aW9uOiBjZW50ZXIgY2VudGVyO1xufVxuQG1peGluIHBzZXVkbyAoJGRpc3BsYXk6IGJsb2NrLCAkcG9zOiBhYnNvbHV0ZSwgJGNvbnRlbnQ6ICcnKXtcbiAgY29udGVudDogJGNvbnRlbnQ7XG4gIGRpc3BsYXk6ICRkaXNwbGF5O1xuICBwb3NpdGlvbjogJHBvcztcbn1cblxuLypcbiogSW1wbGVtZW50YXRpb24gb2YgdGhlbWVzXG4qL1xuQG1peGluIHRoZW1pZnkoJHRoZW1lczogJHRoZW1lcykge1xuICBAZWFjaCAkdGhlbWUsICRtYXAgaW4gJHRoZW1lcyB7XG4gICAgLnRoZW1lLSN7JHRoZW1lfSAmIHtcbiAgICAgICR0aGVtZS1tYXA6ICgpICFnbG9iYWw7XG4gICAgICBAZWFjaCAka2V5LCAkc3VibWFwIGluICRtYXAge1xuICAgICAgICAkdmFsdWU6IG1hcC1nZXQobWFwLWdldCgkdGhlbWVzLCAkdGhlbWUpLCAnI3ska2V5fScpO1xuICAgICAgICAkdGhlbWUtbWFwOiBtYXAtbWVyZ2UoJHRoZW1lLW1hcCwgKCRrZXk6ICR2YWx1ZSkpICFnbG9iYWw7XG4gICAgICB9XG4gICAgICBAY29udGVudDtcbiAgICAgICR0aGVtZS1tYXA6IG51bGwgIWdsb2JhbDtcbiAgICB9XG4gIH1cbn1cblxuQGZ1bmN0aW9uIHRoZW1lZCgka2V5KSB7XG4gIEByZXR1cm4gbWFwLWdldCgkdGhlbWUtbWFwLCAka2V5KTtcbn1cbiIsIi8qXG4qIEltcGxlbWVudGF0aW9uIG9mIHRoZW1lc1xuKi9cbi5hcHAtY29udGVudCB7XG4gIGRpc3BsYXk6IGZsZXg7XG4gIG92ZXJmbG93LXg6IG92ZXJsYXk7XG4gIG92ZXJmbG93LXk6IGhpZGRlbjtcbiAgd2lkdGg6IDEwMCU7IH1cbiAgLmFwcC1jb250ZW50IC5wcmVsb2FkZXIge1xuICAgIGFsaWduLXNlbGY6IGNlbnRlcjtcbiAgICBjb2xvcjogI2ZmZjtcbiAgICBmb250LXNpemU6IDJyZW07XG4gICAgbWFyZ2luOiAwIGF1dG87XG4gICAgdGV4dC1hbGlnbjogY2VudGVyO1xuICAgIHdpZHRoOiA1MCU7IH1cbiAgICAuYXBwLWNvbnRlbnQgLnByZWxvYWRlciAubG9hZGluZy1iYXIge1xuICAgICAgZGlzcGxheTogYmxvY2s7XG4gICAgICBhbmltYXRpb246IG1vdmUgNXMgbGluZWFyIGluZmluaXRlO1xuICAgICAgYmFja2dyb3VuZC1pbWFnZTogLXdlYmtpdC1ncmFkaWVudChsaW5lYXIsIDAgMCwgMTAwJSAxMDAlLCBjb2xvci1zdG9wKDAuMTI1LCByZ2JhKDAsIDAsIDAsIDAuMTUpKSwgY29sb3Itc3RvcCgwLjEyNSwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKDAuMjUsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCgwLjI1LCByZ2JhKDAsIDAsIDAsIDAuMSkpLCBjb2xvci1zdG9wKDAuMzc1LCByZ2JhKDAsIDAsIDAsIDAuMSkpLCBjb2xvci1zdG9wKDAuMzc1LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC41LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC41LCByZ2JhKDAsIDAsIDAsIDAuMTUpKSwgY29sb3Itc3RvcCgwLjYyNSwgcmdiYSgwLCAwLCAwLCAwLjE1KSksIGNvbG9yLXN0b3AoMC42MjUsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCgwLjc1LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC43NSwgcmdiYSgwLCAwLCAwLCAwLjEpKSwgY29sb3Itc3RvcCgwLjg3NSwgcmdiYSgwLCAwLCAwLCAwLjEpKSwgY29sb3Itc3RvcCgwLjg3NSwgdHJhbnNwYXJlbnQpLCB0byh0cmFuc3BhcmVudCkpLCAtd2Via2l0LWdyYWRpZW50KGxpbmVhciwgMCAxMDAlLCAxMDAlIDAsIGNvbG9yLXN0b3AoMC4xMjUsIHJnYmEoMCwgMCwgMCwgMC4zKSksIGNvbG9yLXN0b3AoMC4xMjUsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCgwLjI1LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC4yNSwgcmdiYSgwLCAwLCAwLCAwLjI1KSksIGNvbG9yLXN0b3AoMC4zNzUsIHJnYmEoMCwgMCwgMCwgMC4yNSkpLCBjb2xvci1zdG9wKDAuMzc1LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC41LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC41LCByZ2JhKDAsIDAsIDAsIDAuMykpLCBjb2xvci1zdG9wKDAuNjI1LCByZ2JhKDAsIDAsIDAsIDAuMykpLCBjb2xvci1zdG9wKDAuNjI1LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC43NSwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKDAuNzUsIHJnYmEoMCwgMCwgMCwgMC4yNSkpLCBjb2xvci1zdG9wKDAuODc1LCByZ2JhKDAsIDAsIDAsIDAuMjUpKSwgY29sb3Itc3RvcCgwLjg3NSwgdHJhbnNwYXJlbnQpLCB0byh0cmFuc3BhcmVudCkpO1xuICAgICAgYmFja2dyb3VuZC1zaXplOiAxMHJlbSAxMHJlbTtcbiAgICAgIG1hcmdpbi10b3A6IDJyZW07XG4gICAgICB3aWR0aDogMTAwJTtcbiAgICAgIGhlaWdodDogMXJlbTsgfVxuXG5Aa2V5ZnJhbWVzIG1vdmUge1xuICAwJSB7XG4gICAgYmFja2dyb3VuZC1wb3NpdGlvbjogMTAwJSAtMTByZW07IH1cbiAgMTAwJSB7XG4gICAgYmFja2dyb3VuZC1wb3NpdGlvbjogMTAwJSAxMHJlbTsgfSB9XG4iLCJAaW1wb3J0ICd+c3JjL2Fzc2V0cy9zY3NzL2Jhc2UvbWl4aW5zJztcblxuLmFwcC1jb250ZW50IHtcbiAgZGlzcGxheTogZmxleDtcbiAgb3ZlcmZsb3cteDogb3ZlcmxheTtcbiAgb3ZlcmZsb3cteTogaGlkZGVuO1xuICB3aWR0aDogMTAwJTtcblxuICAucHJlbG9hZGVyIHtcbiAgICBhbGlnbi1zZWxmOiBjZW50ZXI7XG4gICAgY29sb3I6ICNmZmY7XG4gICAgZm9udC1zaXplOiAycmVtO1xuICAgIG1hcmdpbjogMCBhdXRvO1xuICAgIHRleHQtYWxpZ246IGNlbnRlcjtcbiAgICB3aWR0aDogNTAlO1xuXG4gICAgLmxvYWRpbmctYmFyIHtcbiAgICAgIGRpc3BsYXk6IGJsb2NrO1xuICAgICAgYW5pbWF0aW9uOiBtb3ZlIDVzIGxpbmVhciBpbmZpbml0ZTtcbiAgICAgIGJhY2tncm91bmQtaW1hZ2U6XG4gICAgICAgIC13ZWJraXQtZ3JhZGllbnQoXG4gICAgICAgICAgICBsaW5lYXIsIDAgMCwgMTAwJSAxMDAlLFxuICAgICAgICAgICAgY29sb3Itc3RvcCguMTI1LCByZ2JhKDAsIDAsIDAsIC4xNSkpLCBjb2xvci1zdG9wKC4xMjUsIHRyYW5zcGFyZW50KSxcbiAgICAgICAgICAgIGNvbG9yLXN0b3AoLjI1MCwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKC4yNTAsIHJnYmEoMCwgMCwgMCwgLjEwKSksXG4gICAgICAgICAgICBjb2xvci1zdG9wKC4zNzUsIHJnYmEoMCwgMCwgMCwgLjEwKSksIGNvbG9yLXN0b3AoLjM3NSwgdHJhbnNwYXJlbnQpLFxuICAgICAgICAgICAgY29sb3Itc3RvcCguNTAwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjUwMCwgcmdiYSgwLCAwLCAwLCAuMTUpKSxcbiAgICAgICAgICAgIGNvbG9yLXN0b3AoLjYyNSwgcmdiYSgwLCAwLCAwLCAuMTUpKSwgY29sb3Itc3RvcCguNjI1LCB0cmFuc3BhcmVudCksXG4gICAgICAgICAgICBjb2xvci1zdG9wKC43NTAsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCguNzUwLCByZ2JhKDAsIDAsIDAsIC4xMCkpLFxuICAgICAgICAgICAgY29sb3Itc3RvcCguODc1LCByZ2JhKDAsIDAsIDAsIC4xMCkpLCBjb2xvci1zdG9wKC44NzUsIHRyYW5zcGFyZW50KSxcbiAgICAgICAgICAgIHRvKHRyYW5zcGFyZW50KVxuICAgICAgICApLFxuICAgICAgICAtd2Via2l0LWdyYWRpZW50KFxuICAgICAgICAgICAgbGluZWFyLCAwIDEwMCUsIDEwMCUgMCxcbiAgICAgICAgICAgIGNvbG9yLXN0b3AoLjEyNSwgcmdiYSgwLCAwLCAwLCAuMzApKSwgY29sb3Itc3RvcCguMTI1LCB0cmFuc3BhcmVudCksXG4gICAgICAgICAgICBjb2xvci1zdG9wKC4yNTAsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCguMjUwLCByZ2JhKDAsIDAsIDAsIC4yNSkpLFxuICAgICAgICAgICAgY29sb3Itc3RvcCguMzc1LCByZ2JhKDAsIDAsIDAsIC4yNSkpLCBjb2xvci1zdG9wKC4zNzUsIHRyYW5zcGFyZW50KSxcbiAgICAgICAgICAgIGNvbG9yLXN0b3AoLjUwMCwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKC41MDAsIHJnYmEoMCwgMCwgMCwgLjMwKSksXG4gICAgICAgICAgICBjb2xvci1zdG9wKC42MjUsIHJnYmEoMCwgMCwgMCwgLjMwKSksIGNvbG9yLXN0b3AoLjYyNSwgdHJhbnNwYXJlbnQpLFxuICAgICAgICAgICAgY29sb3Itc3RvcCguNzUwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjc1MCwgcmdiYSgwLCAwLCAwLCAuMjUpKSxcbiAgICAgICAgICAgIGNvbG9yLXN0b3AoLjg3NSwgcmdiYSgwLCAwLCAwLCAuMjUpKSwgY29sb3Itc3RvcCguODc1LCB0cmFuc3BhcmVudCksXG4gICAgICAgICAgICB0byh0cmFuc3BhcmVudClcbiAgICAgICAgKTtcbiAgICAgIGJhY2tncm91bmQtc2l6ZTogMTByZW0gMTByZW07XG4gICAgICBtYXJnaW4tdG9wOiAycmVtO1xuICAgICAgd2lkdGg6IDEwMCU7XG4gICAgICBoZWlnaHQ6IDFyZW07XG4gICAgfVxuICB9XG5cbiAgQGtleWZyYW1lcyBtb3ZlIHtcbiAgICAwJSB7XG4gICAgICBiYWNrZ3JvdW5kLXBvc2l0aW9uOiAxMDAlIC0xMHJlbTtcbiAgICB9XG4gICAgMTAwJSB7XG4gICAgICBiYWNrZ3JvdW5kLXBvc2l0aW9uOiAxMDAlIDEwcmVtO1xuICAgIH1cbiAgfVxufVxuIl19 */"

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
/* harmony import */ var ngx_contextmenu__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! ngx-contextmenu */ "./node_modules/ngx-contextmenu/fesm5/ngx-contextmenu.js");
/* harmony import */ var _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! ./_helpers/pipes/int-to-money.pipe */ "./src/app/_helpers/pipes/int-to-money.pipe.ts");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_8__ = __webpack_require__(/*! bignumber.js */ "./node_modules/bignumber.js/bignumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_8___default = /*#__PURE__*/__webpack_require__.n(bignumber_js__WEBPACK_IMPORTED_MODULE_8__);
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_9__ = __webpack_require__(/*! ./_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
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
    function AppComponent(http, renderer, translate, backend, router, variablesService, ngZone, intToMoneyPipe, modalService) {
        var _this = this;
        this.http = http;
        this.renderer = renderer;
        this.translate = translate;
        this.backend = backend;
        this.router = router;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.intToMoneyPipe = intToMoneyPipe;
        this.modalService = modalService;
        this.onQuitRequest = false;
        this.firstOnlineState = false;
        this.translateUsed = false;
        this.needOpenWallets = [];
        translate.addLangs(['en', 'fr']);
        translate.setDefaultLang('en');
        // const browserLang = translate.getBrowserLang();
        // translate.use(browserLang.match(/en|fr/) ? browserLang : 'en');
        translate.use('en').subscribe(function () {
            _this.translateUsed = true;
        });
    }
    AppComponent.prototype.setBackendLocalization = function () {
        var _this = this;
        if (this.translateUsed) {
            var stringsArray = [
                this.translate.instant('BACKEND_LOCALIZATION.QUIT'),
                this.translate.instant('BACKEND_LOCALIZATION.IS_RECEIVED'),
                this.translate.instant('BACKEND_LOCALIZATION.IS_CONFIRMED'),
                this.translate.instant('BACKEND_LOCALIZATION.INCOME_TRANSFER_UNCONFIRMED'),
                this.translate.instant('BACKEND_LOCALIZATION.INCOME_TRANSFER_CONFIRMED'),
                this.translate.instant('BACKEND_LOCALIZATION.MINED'),
                this.translate.instant('BACKEND_LOCALIZATION.LOCKED'),
                this.translate.instant('BACKEND_LOCALIZATION.IS_MINIMIZE'),
                this.translate.instant('BACKEND_LOCALIZATION.RESTORE'),
                this.translate.instant('BACKEND_LOCALIZATION.TRAY_MENU_SHOW'),
                this.translate.instant('BACKEND_LOCALIZATION.TRAY_MENU_MINIMIZE')
            ];
            this.backend.setBackendLocalization(stringsArray, 'en');
        }
        else {
            console.warn('wait translate use');
            setTimeout(function () {
                _this.setBackendLocalization();
            }, 10000);
        }
    };
    AppComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.variablesService.allContextMenu = this.allContextMenu;
        this.variablesService.onlyCopyContextMenu = this.onlyCopyContextMenu;
        this.backend.initService().subscribe(function (initMessage) {
            console.log('Init message: ', initMessage);
            _this.backend.webkitLaunchedScript();
            _this.backend.start_backend(false, '127.0.0.1', 11512, function (st2, dd2) {
                console.log(st2, dd2);
            });
            _this.backend.eventSubscribe('quit_requested', function () {
                if (!_this.onQuitRequest) {
                    _this.ngZone.run(function () {
                        _this.router.navigate(['/']);
                    });
                    _this.needOpenWallets = [];
                    _this.variablesService.daemon_state = 5;
                    var saveFunction_1 = function () {
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
                    };
                    if (_this.variablesService.appPass) {
                        _this.backend.storeSecureAppData(function () {
                            saveFunction_1();
                        });
                    }
                    else {
                        saveFunction_1();
                    }
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
                            // wallet.error = true;
                        }
                        wallet.balance = data.balance;
                        wallet.unlocked_balance = data.unlocked_balance;
                        wallet.mined_total = data.minied_total;
                        wallet.alias_available = data.is_alias_operations_available;
                    });
                }
            });
            _this.backend.eventSubscribe('wallet_sync_progress', function (data) {
                console.log('----------------- wallet_sync_progress -----------------');
                console.log(data);
                var wallet = _this.variablesService.getWallet(data.wallet_id);
                if (wallet) {
                    _this.ngZone.run(function () {
                        wallet.progress = (data.progress < 0) ? 0 : ((data.progress > 100) ? 100 : data.progress);
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
                // this.variablesService.exp_med_ts = data['expiration_median_timestamp'] + 600 + 1;
                _this.variablesService.setExpMedTs(data['expiration_median_timestamp'] + 600 + 1);
                _this.variablesService.net_time_delta_median = data.net_time_delta_median;
                _this.variablesService.last_build_available = data.last_build_available;
                _this.variablesService.last_build_displaymode = data.last_build_displaymode;
                _this.variablesService.setHeightApp(data.height);
                _this.ngZone.run(function () {
                    _this.variablesService.daemon_state = data['daemon_network_state'];
                    if (data['daemon_network_state'] === 1) {
                        var max = data['max_net_seen_height'] - data['synchronization_start_height'];
                        var current = data.height - data['synchronization_start_height'];
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
                if (!_this.firstOnlineState && data['daemon_network_state'] === 2) {
                    _this.getAliases();
                    _this.backend.getDefaultFee(function (status_fee, data_fee) {
                        _this.variablesService.default_fee_big = new bignumber_js__WEBPACK_IMPORTED_MODULE_8__["BigNumber"](data_fee);
                        _this.variablesService.default_fee = _this.intToMoneyPipe.transform(data_fee);
                    });
                    _this.firstOnlineState = true;
                }
            });
            _this.backend.eventSubscribe('money_transfer', function (data) {
                console.log('----------------- money_transfer -----------------');
                console.log(data);
                if (!data.ti) {
                    return;
                }
                var wallet_id = data.wallet_id;
                var tr_info = data.ti;
                var wallet = _this.variablesService.getWallet(wallet_id);
                if (wallet) {
                    _this.ngZone.run(function () {
                        if (!wallet.loaded) {
                            wallet.balance = data.balance;
                            wallet.unlocked_balance = data.unlocked_balance;
                        }
                        else {
                            wallet.balance = data.balance;
                            wallet.unlocked_balance = data.unlocked_balance;
                        }
                        if (tr_info.tx_type === 6) {
                            _this.variablesService.setRefreshStacking(wallet_id);
                        }
                        var tr_exists = wallet.excluded_history.some(function (elem) { return elem.tx_hash === tr_info.tx_hash; });
                        tr_exists = (!tr_exists) ? wallet.history.some(function (elem) { return elem.tx_hash === tr_info.tx_hash; }) : tr_exists;
                        wallet.prepareHistory([tr_info]);
                        if (tr_info.hasOwnProperty('contract')) {
                            var exp_med_ts = _this.variablesService.exp_med_ts;
                            var height_app = _this.variablesService.height_app;
                            var contract_1 = tr_info.contract[0];
                            if (tr_exists) {
                                for (var i = 0; i < wallet.contracts.length; i++) {
                                    if (wallet.contracts[i].contract_id === contract_1.contract_id && wallet.contracts[i].is_a === contract_1.is_a) {
                                        wallet.contracts[i].cancel_expiration_time = contract_1.cancel_expiration_time;
                                        wallet.contracts[i].expiration_time = contract_1.expiration_time;
                                        wallet.contracts[i].height = contract_1.height;
                                        wallet.contracts[i].timestamp = contract_1.timestamp;
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
                            var findContract = false;
                            for (var i = 0; i < wallet.contracts.length; i++) {
                                if (wallet.contracts[i].contract_id === contract_1.contract_id && wallet.contracts[i].is_a === contract_1.is_a) {
                                    for (var prop in contract_1) {
                                        if (contract_1.hasOwnProperty(prop)) {
                                            wallet.contracts[i][prop] = contract_1[prop];
                                        }
                                    }
                                    findContract = true;
                                    break;
                                }
                            }
                            if (findContract === false) {
                                wallet.contracts.push(contract_1);
                            }
                            wallet.recountNewContracts();
                        }
                    });
                }
            });
            _this.backend.eventSubscribe('money_transfer_cancel', function (data) {
                console.log('----------------- money_transfer_cancel -----------------');
                console.log(data);
                if (!data.ti) {
                    return;
                }
                var wallet_id = data.wallet_id;
                var tr_info = data.ti;
                var wallet = _this.variablesService.getWallet(wallet_id);
                if (wallet) {
                    if (tr_info.hasOwnProperty('contract')) {
                        for (var i = 0; i < wallet.contracts.length; i++) {
                            if (wallet.contracts[i].contract_id === tr_info.contract[0].contract_id && wallet.contracts[i].is_a === tr_info.contract[0].is_a) {
                                if (wallet.contracts[i].state === 1 || wallet.contracts[i].state === 110) {
                                    wallet.contracts[i].is_new = true;
                                    wallet.contracts[i].state = 140;
                                    wallet.recountNewContracts();
                                }
                                break;
                            }
                        }
                    }
                    wallet.removeFromHistory(tr_info.tx_hash);
                    var error_tr = '';
                    switch (tr_info.tx_type) {
                        case 0:
                            error_tr = _this.translate.instant('ERRORS.TX_TYPE_NORMAL') + '<br>' +
                                tr_info.tx_hash + '<br>' + wallet.name + '<br>' + wallet.address + '<br>' +
                                _this.translate.instant('ERRORS.TX_TYPE_NORMAL_TO') + ' ' + _this.intToMoneyPipe.transform(tr_info.amount) + ' ' +
                                _this.translate.instant('ERRORS.TX_TYPE_NORMAL_END');
                            break;
                        case 1:
                            // this.translate.instant('ERRORS.TX_TYPE_PUSH_OFFER');
                            break;
                        case 2:
                            // this.translate.instant('ERRORS.TX_TYPE_UPDATE_OFFER');
                            break;
                        case 3:
                            // this.translate.instant('ERRORS.TX_TYPE_CANCEL_OFFER');
                            break;
                        case 4:
                            error_tr = _this.translate.instant('ERRORS.TX_TYPE_NEW_ALIAS') + '<br>' +
                                tr_info.tx_hash + '<br>' + wallet.name + '<br>' + wallet.address + '<br>' +
                                _this.translate.instant('ERRORS.TX_TYPE_NEW_ALIAS_END');
                            break;
                        case 5:
                            error_tr = _this.translate.instant('ERRORS.TX_TYPE_UPDATE_ALIAS') + '<br>' +
                                tr_info.tx_hash + '<br>' + wallet.name + '<br>' + wallet.address + '<br>' +
                                _this.translate.instant('ERRORS.TX_TYPE_NEW_ALIAS_END');
                            break;
                        case 6:
                            error_tr = _this.translate.instant('ERRORS.TX_TYPE_COIN_BASE');
                            break;
                    }
                    if (error_tr) {
                        _this.modalService.prepareModal('error', error_tr);
                    }
                }
            });
            _this.backend.eventSubscribe('on_core_event', function (data) {
                console.log('----------------- on_core_event -----------------');
                console.log(data);
                data = JSON.parse(data);
                if (data.events != null) {
                    var _loop_1 = function (i, length_1) {
                        switch (data.events[i].method) {
                            case 'CORE_EVENT_BLOCK_ADDED':
                                break;
                            case 'CORE_EVENT_ADD_ALIAS':
                                if (_this.variablesService.aliasesChecked[data.events[i].details.address] != null) {
                                    _this.variablesService.aliasesChecked[data.events[i].details.address]['name'] = '@' + data.events[i].details.alias;
                                    _this.variablesService.aliasesChecked[data.events[i].details.address]['address'] = data.events[i].details.address;
                                    _this.variablesService.aliasesChecked[data.events[i].details.address]['comment'] = data.events[i].details.comment;
                                }
                                if (_this.variablesService.enableAliasSearch) {
                                    var newAlias = {
                                        name: '@' + data.events[i].details.alias,
                                        address: data.events[i].details.address,
                                        comment: data.events[i].details.comment
                                    };
                                    _this.variablesService.aliases = _this.variablesService.aliases.concat(newAlias);
                                    _this.variablesService.changeAliases();
                                }
                                break;
                            case 'CORE_EVENT_UPDATE_ALIAS':
                                for (var address in _this.variablesService.aliasesChecked) {
                                    if (_this.variablesService.aliasesChecked.hasOwnProperty(address)) {
                                        if (_this.variablesService.aliasesChecked[address].name === '@' + data.events[i].details.alias) {
                                            if (_this.variablesService.aliasesChecked[address].address !== data.events[i].details.details.address) {
                                                delete _this.variablesService.aliasesChecked[address]['name'];
                                                delete _this.variablesService.aliasesChecked[address]['address'];
                                                delete _this.variablesService.aliasesChecked[address]['comment'];
                                            }
                                            else {
                                                _this.variablesService.aliasesChecked[address].comment = data.events[i].details.details.comment;
                                            }
                                            break;
                                        }
                                    }
                                }
                                if (_this.variablesService.aliasesChecked[data.events[i].details.details.address] != null) {
                                    _this.variablesService.aliasesChecked[data.events[i].details.details.address]['name'] = '@' + data.events[i].details.alias;
                                    _this.variablesService.aliasesChecked[data.events[i].details.details.address]['address'] = data.events[i].details.details.address;
                                    _this.variablesService.aliasesChecked[data.events[i].details.details.address]['comment'] = data.events[i].details.details.comment;
                                }
                                if (_this.variablesService.enableAliasSearch) {
                                    var CurrentAlias = _this.variablesService.aliases.find(function (element) { return element.name === '@' + data.events[i].details.alias; });
                                    if (CurrentAlias) {
                                        CurrentAlias.address = data.events[i].details.details.address;
                                        CurrentAlias.comment = data.events[i].details.details.comment;
                                    }
                                }
                                _this.variablesService.changeAliases();
                                break;
                            default:
                                break;
                        }
                    };
                    for (var i = 0, length_1 = data.events.length; i < length_1; i++) {
                        _loop_1(i, length_1);
                    }
                }
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
            _this.expMedTsEvent = _this.variablesService.getExpMedTsEvent.subscribe(function (newTimestamp) {
                _this.variablesService.wallets.forEach(function (wallet) {
                    wallet.contracts.forEach(function (contract) {
                        if (contract.state === 1 && contract.expiration_time <= newTimestamp) {
                            contract.state = 110;
                            contract.is_new = true;
                            wallet.recountNewContracts();
                        }
                        else if (contract.state === 5 && contract.cancel_expiration_time <= newTimestamp) {
                            contract.state = 130;
                            contract.is_new = true;
                            wallet.recountNewContracts();
                        }
                    });
                });
            });
            _this.backend.getAppData(function (status, data) {
                if (data && Object.keys(data).length > 0) {
                    for (var key in data) {
                        if (data.hasOwnProperty(key) && _this.variablesService.settings.hasOwnProperty(key)) {
                            _this.variablesService.settings[key] = data[key];
                        }
                    }
                    if (_this.variablesService.settings.hasOwnProperty('theme') && ['dark', 'white', 'gray'].indexOf(_this.variablesService.settings.theme) !== -1) {
                        _this.renderer.addClass(document.body, 'theme-' + _this.variablesService.settings.theme);
                    }
                    else {
                        _this.renderer.addClass(document.body, 'theme-' + _this.variablesService.defaultTheme);
                    }
                    if (_this.variablesService.settings.hasOwnProperty('scale') && [7.5, 10, 12.5, 15].indexOf(_this.variablesService.settings.scale) !== -1) {
                        _this.renderer.setStyle(document.documentElement, 'font-size', _this.variablesService.settings.scale + 'px');
                    }
                }
                else {
                    _this.variablesService.settings.theme = _this.variablesService.defaultTheme;
                    _this.renderer.addClass(document.body, 'theme-' + _this.variablesService.settings.theme);
                }
                _this.setBackendLocalization();
                _this.backend.setLogLevel(_this.variablesService.settings.appLog);
                if (_this.router.url !== '/login') {
                    _this.backend.haveSecureAppData(function (statusPass) {
                        if (statusPass) {
                            _this.ngZone.run(function () {
                                _this.router.navigate(['/login'], { queryParams: { type: 'auth' } });
                            });
                        }
                        else {
                            if (Object.keys(data).length !== 0) {
                                _this.needOpenWallets = JSON.parse(JSON.stringify(_this.variablesService.settings.wallets));
                                _this.ngZone.run(function () {
                                    _this.variablesService.appLogin = true;
                                    _this.router.navigate(['/']);
                                });
                            }
                            else {
                                _this.ngZone.run(function () {
                                    _this.router.navigate(['/login'], { queryParams: { type: 'reg' } });
                                });
                            }
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
        this.http.get('https://api.coingecko.com/api/v3/ping').subscribe(function () {
            _this.http.get('https://api.coingecko.com/api/v3/simple/price?ids=zano&vs_currencies=usd').subscribe(function (data) {
                _this.variablesService.moneyEquivalent = data['zano']['usd'];
            }, function (error) {
                console.warn('api.coingecko.com price error: ', error);
            });
        }, function (error) {
            console.warn('api.coingecko.com error: ', error);
            setTimeout(function () {
                _this.getMoneyEquivalent();
            }, 60000);
        });
    };
    AppComponent.prototype.getAliases = function () {
        var _this = this;
        this.backend.getAllAliases(function (status, data, error) {
            console.warn(error);
            if (error === 'CORE_BUSY') {
                window.setTimeout(function () {
                    _this.getAliases();
                }, 10000);
            }
            else if (error === 'OVERFLOW') {
                _this.variablesService.aliases = [];
                _this.variablesService.enableAliasSearch = false;
                _this.variablesService.wallets.forEach(function (wallet) {
                    wallet.alias = _this.backend.getWalletAlias(wallet.address);
                });
            }
            else {
                _this.variablesService.enableAliasSearch = true;
                if (data.aliases && data.aliases.length) {
                    _this.variablesService.aliases = [];
                    data.aliases.forEach(function (alias) {
                        var newAlias = {
                            name: '@' + alias.alias,
                            address: alias.address,
                            comment: alias.comment
                        };
                        _this.variablesService.aliases.push(newAlias);
                    });
                    _this.variablesService.wallets.forEach(function (wallet) {
                        wallet.alias = _this.backend.getWalletAlias(wallet.address);
                    });
                    _this.variablesService.aliases = _this.variablesService.aliases.sort(function (a, b) {
                        if (a.name.length > b.name.length) {
                            return 1;
                        }
                        if (a.name.length < b.name.length) {
                            return -1;
                        }
                        if (a.name > b.name) {
                            return 1;
                        }
                        if (a.name < b.name) {
                            return -1;
                        }
                        return 0;
                    });
                    _this.variablesService.changeAliases();
                }
            }
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
    AppComponent.prototype.contextMenuOnlyCopy = function (text) {
        if (text) {
            this.backend.setClipboard(String(text));
        }
    };
    AppComponent.prototype.contextMenuPaste = function (target) {
        if (target && (target['nodeName'].toUpperCase() === 'TEXTAREA' || target['nodeName'].toUpperCase() === 'INPUT')) {
            this.backend.getClipboard(function (status, clipboard) {
                clipboard = String(clipboard);
                if (typeof clipboard !== 'string' || clipboard.length) {
                    var start = (target['contextSelectionStart']) ? 'contextSelectionStart' : 'selectionStart';
                    var end = (target['contextSelectionEnd']) ? 'contextSelectionEnd' : 'selectionEnd';
                    var _pre = target['value'].substring(0, target[start]);
                    var _aft = target['value'].substring(target[end], target['value'].length);
                    var text = _pre + clipboard + _aft;
                    var cursorPosition = (_pre + clipboard).length;
                    if (target['maxLength'] && parseInt(target['maxLength'], 10) > 0) {
                        text = text.substr(0, parseInt(target['maxLength'], 10));
                    }
                    target['value'] = text;
                    target.setSelectionRange(cursorPosition, cursorPosition);
                    target.dispatchEvent(new Event('input'));
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
        this.expMedTsEvent.unsubscribe();
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["ViewChild"])('allContextMenu'),
        __metadata("design:type", ngx_contextmenu__WEBPACK_IMPORTED_MODULE_6__["ContextMenuComponent"])
    ], AppComponent.prototype, "allContextMenu", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["ViewChild"])('onlyCopyContextMenu'),
        __metadata("design:type", ngx_contextmenu__WEBPACK_IMPORTED_MODULE_6__["ContextMenuComponent"])
    ], AppComponent.prototype, "onlyCopyContextMenu", void 0);
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
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_7__["IntToMoneyPipe"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_9__["ModalService"]])
    ], AppComponent);
    return AppComponent;
}());



/***/ }),

/***/ "./src/app/app.module.ts":
/*!*******************************!*\
  !*** ./src/app/app.module.ts ***!
  \*******************************/
/*! exports provided: HttpLoaderFactory, highchartsFactory, AppModule */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "HttpLoaderFactory", function() { return HttpLoaderFactory; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "highchartsFactory", function() { return highchartsFactory; });
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
/* harmony import */ var _open_wallet_modal_open_wallet_modal_component__WEBPACK_IMPORTED_MODULE_10__ = __webpack_require__(/*! ./open-wallet-modal/open-wallet-modal.component */ "./src/app/open-wallet-modal/open-wallet-modal.component.ts");
/* harmony import */ var _restore_wallet_restore_wallet_component__WEBPACK_IMPORTED_MODULE_11__ = __webpack_require__(/*! ./restore-wallet/restore-wallet.component */ "./src/app/restore-wallet/restore-wallet.component.ts");
/* harmony import */ var _seed_phrase_seed_phrase_component__WEBPACK_IMPORTED_MODULE_12__ = __webpack_require__(/*! ./seed-phrase/seed-phrase.component */ "./src/app/seed-phrase/seed-phrase.component.ts");
/* harmony import */ var _wallet_details_wallet_details_component__WEBPACK_IMPORTED_MODULE_13__ = __webpack_require__(/*! ./wallet-details/wallet-details.component */ "./src/app/wallet-details/wallet-details.component.ts");
/* harmony import */ var _assign_alias_assign_alias_component__WEBPACK_IMPORTED_MODULE_14__ = __webpack_require__(/*! ./assign-alias/assign-alias.component */ "./src/app/assign-alias/assign-alias.component.ts");
/* harmony import */ var _edit_alias_edit_alias_component__WEBPACK_IMPORTED_MODULE_15__ = __webpack_require__(/*! ./edit-alias/edit-alias.component */ "./src/app/edit-alias/edit-alias.component.ts");
/* harmony import */ var _transfer_alias_transfer_alias_component__WEBPACK_IMPORTED_MODULE_16__ = __webpack_require__(/*! ./transfer-alias/transfer-alias.component */ "./src/app/transfer-alias/transfer-alias.component.ts");
/* harmony import */ var _wallet_wallet_component__WEBPACK_IMPORTED_MODULE_17__ = __webpack_require__(/*! ./wallet/wallet.component */ "./src/app/wallet/wallet.component.ts");
/* harmony import */ var _send_send_component__WEBPACK_IMPORTED_MODULE_18__ = __webpack_require__(/*! ./send/send.component */ "./src/app/send/send.component.ts");
/* harmony import */ var _receive_receive_component__WEBPACK_IMPORTED_MODULE_19__ = __webpack_require__(/*! ./receive/receive.component */ "./src/app/receive/receive.component.ts");
/* harmony import */ var _history_history_component__WEBPACK_IMPORTED_MODULE_20__ = __webpack_require__(/*! ./history/history.component */ "./src/app/history/history.component.ts");
/* harmony import */ var _contracts_contracts_component__WEBPACK_IMPORTED_MODULE_21__ = __webpack_require__(/*! ./contracts/contracts.component */ "./src/app/contracts/contracts.component.ts");
/* harmony import */ var _purchase_purchase_component__WEBPACK_IMPORTED_MODULE_22__ = __webpack_require__(/*! ./purchase/purchase.component */ "./src/app/purchase/purchase.component.ts");
/* harmony import */ var _messages_messages_component__WEBPACK_IMPORTED_MODULE_23__ = __webpack_require__(/*! ./messages/messages.component */ "./src/app/messages/messages.component.ts");
/* harmony import */ var _typing_message_typing_message_component__WEBPACK_IMPORTED_MODULE_24__ = __webpack_require__(/*! ./typing-message/typing-message.component */ "./src/app/typing-message/typing-message.component.ts");
/* harmony import */ var _staking_staking_component__WEBPACK_IMPORTED_MODULE_25__ = __webpack_require__(/*! ./staking/staking.component */ "./src/app/staking/staking.component.ts");
/* harmony import */ var _angular_common_http__WEBPACK_IMPORTED_MODULE_26__ = __webpack_require__(/*! @angular/common/http */ "./node_modules/@angular/common/fesm5/http.js");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_27__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
/* harmony import */ var _ngx_translate_http_loader__WEBPACK_IMPORTED_MODULE_28__ = __webpack_require__(/*! @ngx-translate/http-loader */ "./node_modules/@ngx-translate/http-loader/fesm5/ngx-translate-http-loader.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_29__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _ng_select_ng_select__WEBPACK_IMPORTED_MODULE_30__ = __webpack_require__(/*! @ng-select/ng-select */ "./node_modules/@ng-select/ng-select/fesm5/ng-select.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_31__ = __webpack_require__(/*! ./_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_32__ = __webpack_require__(/*! ./_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var _helpers_pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_33__ = __webpack_require__(/*! ./_helpers/pipes/money-to-int.pipe */ "./src/app/_helpers/pipes/money-to-int.pipe.ts");
/* harmony import */ var _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_34__ = __webpack_require__(/*! ./_helpers/pipes/int-to-money.pipe */ "./src/app/_helpers/pipes/int-to-money.pipe.ts");
/* harmony import */ var _helpers_pipes_history_type_messages_pipe__WEBPACK_IMPORTED_MODULE_35__ = __webpack_require__(/*! ./_helpers/pipes/history-type-messages.pipe */ "./src/app/_helpers/pipes/history-type-messages.pipe.ts");
/* harmony import */ var _helpers_pipes_contract_status_messages_pipe__WEBPACK_IMPORTED_MODULE_36__ = __webpack_require__(/*! ./_helpers/pipes/contract-status-messages.pipe */ "./src/app/_helpers/pipes/contract-status-messages.pipe.ts");
/* harmony import */ var _helpers_pipes_contract_time_left_pipe__WEBPACK_IMPORTED_MODULE_37__ = __webpack_require__(/*! ./_helpers/pipes/contract-time-left.pipe */ "./src/app/_helpers/pipes/contract-time-left.pipe.ts");
/* harmony import */ var _helpers_directives_tooltip_directive__WEBPACK_IMPORTED_MODULE_38__ = __webpack_require__(/*! ./_helpers/directives/tooltip.directive */ "./src/app/_helpers/directives/tooltip.directive.ts");
/* harmony import */ var _helpers_directives_input_validate_input_validate_directive__WEBPACK_IMPORTED_MODULE_39__ = __webpack_require__(/*! ./_helpers/directives/input-validate/input-validate.directive */ "./src/app/_helpers/directives/input-validate/input-validate.directive.ts");
/* harmony import */ var _helpers_directives_staking_switch_staking_switch_component__WEBPACK_IMPORTED_MODULE_40__ = __webpack_require__(/*! ./_helpers/directives/staking-switch/staking-switch.component */ "./src/app/_helpers/directives/staking-switch/staking-switch.component.ts");
/* harmony import */ var _helpers_directives_modal_container_modal_container_component__WEBPACK_IMPORTED_MODULE_41__ = __webpack_require__(/*! ./_helpers/directives/modal-container/modal-container.component */ "./src/app/_helpers/directives/modal-container/modal-container.component.ts");
/* harmony import */ var _helpers_directives_transaction_details_transaction_details_component__WEBPACK_IMPORTED_MODULE_42__ = __webpack_require__(/*! ./_helpers/directives/transaction-details/transaction-details.component */ "./src/app/_helpers/directives/transaction-details/transaction-details.component.ts");
/* harmony import */ var ngx_contextmenu__WEBPACK_IMPORTED_MODULE_43__ = __webpack_require__(/*! ngx-contextmenu */ "./node_modules/ngx-contextmenu/fesm5/ngx-contextmenu.js");
/* harmony import */ var angular_highcharts__WEBPACK_IMPORTED_MODULE_44__ = __webpack_require__(/*! angular-highcharts */ "./node_modules/angular-highcharts/fesm5/angular-highcharts.js");
/* harmony import */ var highcharts__WEBPACK_IMPORTED_MODULE_45__ = __webpack_require__(/*! highcharts */ "./node_modules/highcharts/highcharts.js");
/* harmony import */ var highcharts__WEBPACK_IMPORTED_MODULE_45___default = /*#__PURE__*/__webpack_require__.n(highcharts__WEBPACK_IMPORTED_MODULE_45__);
/* harmony import */ var highcharts_modules_exporting_src__WEBPACK_IMPORTED_MODULE_46__ = __webpack_require__(/*! highcharts/modules/exporting.src */ "./node_modules/highcharts/modules/exporting.src.js");
/* harmony import */ var highcharts_modules_exporting_src__WEBPACK_IMPORTED_MODULE_46___default = /*#__PURE__*/__webpack_require__.n(highcharts_modules_exporting_src__WEBPACK_IMPORTED_MODULE_46__);
/* harmony import */ var _helpers_directives_progress_container_progress_container_component__WEBPACK_IMPORTED_MODULE_47__ = __webpack_require__(/*! ./_helpers/directives/progress-container/progress-container.component */ "./src/app/_helpers/directives/progress-container/progress-container.component.ts");
/* harmony import */ var _helpers_directives_input_disable_selection_input_disable_selection_directive__WEBPACK_IMPORTED_MODULE_48__ = __webpack_require__(/*! ./_helpers/directives/input-disable-selection/input-disable-selection.directive */ "./src/app/_helpers/directives/input-disable-selection/input-disable-selection.directive.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};

















































function HttpLoaderFactory(httpClient) {
    return new _ngx_translate_http_loader__WEBPACK_IMPORTED_MODULE_28__["TranslateHttpLoader"](httpClient, './assets/i18n/', '.json');
}
// import * as more from 'highcharts/highcharts-more.src';
// import * as exporting from 'highcharts/modules/exporting.src';
// import * as highstock from 'highcharts/modules/stock.src';
function highchartsFactory() {
    // Default options.
    highcharts__WEBPACK_IMPORTED_MODULE_45__["setOptions"]({
        global: {
            useUTC: false
        }
    });
    return [highcharts_modules_exporting_src__WEBPACK_IMPORTED_MODULE_46___default.a];
}
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
                _open_wallet_modal_open_wallet_modal_component__WEBPACK_IMPORTED_MODULE_10__["OpenWalletModalComponent"],
                _restore_wallet_restore_wallet_component__WEBPACK_IMPORTED_MODULE_11__["RestoreWalletComponent"],
                _seed_phrase_seed_phrase_component__WEBPACK_IMPORTED_MODULE_12__["SeedPhraseComponent"],
                _wallet_details_wallet_details_component__WEBPACK_IMPORTED_MODULE_13__["WalletDetailsComponent"],
                _assign_alias_assign_alias_component__WEBPACK_IMPORTED_MODULE_14__["AssignAliasComponent"],
                _edit_alias_edit_alias_component__WEBPACK_IMPORTED_MODULE_15__["EditAliasComponent"],
                _transfer_alias_transfer_alias_component__WEBPACK_IMPORTED_MODULE_16__["TransferAliasComponent"],
                _wallet_wallet_component__WEBPACK_IMPORTED_MODULE_17__["WalletComponent"],
                _send_send_component__WEBPACK_IMPORTED_MODULE_18__["SendComponent"],
                _receive_receive_component__WEBPACK_IMPORTED_MODULE_19__["ReceiveComponent"],
                _history_history_component__WEBPACK_IMPORTED_MODULE_20__["HistoryComponent"],
                _contracts_contracts_component__WEBPACK_IMPORTED_MODULE_21__["ContractsComponent"],
                _purchase_purchase_component__WEBPACK_IMPORTED_MODULE_22__["PurchaseComponent"],
                _messages_messages_component__WEBPACK_IMPORTED_MODULE_23__["MessagesComponent"],
                _staking_staking_component__WEBPACK_IMPORTED_MODULE_25__["StakingComponent"],
                _typing_message_typing_message_component__WEBPACK_IMPORTED_MODULE_24__["TypingMessageComponent"],
                _helpers_pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_33__["MoneyToIntPipe"],
                _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_34__["IntToMoneyPipe"],
                _helpers_directives_staking_switch_staking_switch_component__WEBPACK_IMPORTED_MODULE_40__["StakingSwitchComponent"],
                _helpers_pipes_history_type_messages_pipe__WEBPACK_IMPORTED_MODULE_35__["HistoryTypeMessagesPipe"],
                _helpers_pipes_contract_status_messages_pipe__WEBPACK_IMPORTED_MODULE_36__["ContractStatusMessagesPipe"],
                _helpers_pipes_contract_time_left_pipe__WEBPACK_IMPORTED_MODULE_37__["ContractTimeLeftPipe"],
                _helpers_directives_tooltip_directive__WEBPACK_IMPORTED_MODULE_38__["TooltipDirective"],
                _helpers_directives_input_validate_input_validate_directive__WEBPACK_IMPORTED_MODULE_39__["InputValidateDirective"],
                _helpers_directives_modal_container_modal_container_component__WEBPACK_IMPORTED_MODULE_41__["ModalContainerComponent"],
                _helpers_directives_transaction_details_transaction_details_component__WEBPACK_IMPORTED_MODULE_42__["TransactionDetailsComponent"],
                _helpers_directives_progress_container_progress_container_component__WEBPACK_IMPORTED_MODULE_47__["ProgressContainerComponent"],
                _helpers_directives_input_disable_selection_input_disable_selection_directive__WEBPACK_IMPORTED_MODULE_48__["InputDisableSelectionDirective"]
            ],
            imports: [
                _angular_platform_browser__WEBPACK_IMPORTED_MODULE_0__["BrowserModule"],
                _app_routing_module__WEBPACK_IMPORTED_MODULE_2__["AppRoutingModule"],
                _angular_common_http__WEBPACK_IMPORTED_MODULE_26__["HttpClientModule"],
                _ngx_translate_core__WEBPACK_IMPORTED_MODULE_27__["TranslateModule"].forRoot({
                    loader: {
                        provide: _ngx_translate_core__WEBPACK_IMPORTED_MODULE_27__["TranslateLoader"],
                        useFactory: HttpLoaderFactory,
                        deps: [_angular_common_http__WEBPACK_IMPORTED_MODULE_26__["HttpClient"]]
                    }
                }),
                _angular_forms__WEBPACK_IMPORTED_MODULE_29__["FormsModule"],
                _angular_forms__WEBPACK_IMPORTED_MODULE_29__["ReactiveFormsModule"],
                _ng_select_ng_select__WEBPACK_IMPORTED_MODULE_30__["NgSelectModule"],
                angular_highcharts__WEBPACK_IMPORTED_MODULE_44__["ChartModule"],
                ngx_contextmenu__WEBPACK_IMPORTED_MODULE_43__["ContextMenuModule"].forRoot()
            ],
            providers: [
                _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_31__["BackendService"],
                _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_32__["ModalService"],
                _helpers_pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_33__["MoneyToIntPipe"],
                _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_34__["IntToMoneyPipe"],
                { provide: angular_highcharts__WEBPACK_IMPORTED_MODULE_44__["HIGHCHARTS_MODULES"], useFactory: highchartsFactory }
                // {provide: HIGHCHARTS_MODULES, useFactory: () => [ highstock, more, exporting ] }
            ],
            entryComponents: [
                _helpers_directives_modal_container_modal_container_component__WEBPACK_IMPORTED_MODULE_41__["ModalContainerComponent"]
            ],
            bootstrap: [_app_component__WEBPACK_IMPORTED_MODULE_3__["AppComponent"]]
        })
    ], AppModule);
    return AppModule;
}());



/***/ }),

/***/ "./src/app/assign-alias/assign-alias.component.html":
/*!**********************************************************!*\
  !*** ./src/app/assign-alias/assign-alias.component.html ***!
  \**********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\n\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span [routerLink]=\"['/wallet/' + wallet.wallet_id + '/history']\">{{ wallet.name }}</span>\n      <span>{{ 'BREADCRUMBS.ASSIGN_ALIAS' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <form class=\"form-assign\" [formGroup]=\"assignForm\">\n\n    <div class=\"input-block alias-name\">\n      <label for=\"alias-name\" tooltip=\"{{ 'ASSIGN_ALIAS.NAME.TOOLTIP' | translate }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip assign-alias-tooltip\" [delay]=\"50\">\n        {{ 'ASSIGN_ALIAS.NAME.LABEL' | translate }}\n      </label>\n      <input type=\"text\" id=\"alias-name\" formControlName=\"name\" placeholder=\"{{ 'ASSIGN_ALIAS.NAME.PLACEHOLDER' | translate }}\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"assignForm.controls['name'].invalid && (assignForm.controls['name'].dirty || assignForm.controls['name'].touched)\">\n        <div *ngIf=\"assignForm.controls['name'].errors['required']\">\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NAME_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"assignForm.controls['name'].errors['pattern'] && assignForm.get('name').value.length > 6 && assignForm.get('name').value.length <= 25\">\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NAME_WRONG' | translate }}\n        </div>\n        <div *ngIf=\"assignForm.get('name').value.length <= 6 || assignForm.get('name').value.length > 25\">\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NAME_LENGTH' | translate }}\n        </div>\n      </div>\n      <div class=\"error-block\" *ngIf=\"alias.exists\">\n        <div>\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NAME_EXISTS' | translate }}\n        </div>\n      </div>\n      <div class=\"error-block\" *ngIf=\"notEnoughMoney\">\n        <div>\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NO_MONEY' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"input-block textarea\">\n      <label for=\"alias-comment\" tooltip=\"{{ 'ASSIGN_ALIAS.COMMENT.TOOLTIP' | translate }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip assign-alias-tooltip\" [delay]=\"50\">\n        {{ 'ASSIGN_ALIAS.COMMENT.LABEL' | translate }}\n      </label>\n      <textarea id=\"alias-comment\"\n                class=\"scrolled-content\"\n                formControlName=\"comment\"\n                placeholder=\"{{ 'ASSIGN_ALIAS.COMMENT.PLACEHOLDER' | translate }}\"\n                [maxLength]=\"variablesService.maxCommentLength\"\n                (contextmenu)=\"variablesService.onContextMenu($event)\">\n      </textarea>\n      <div class=\"error-block\" *ngIf=\"assignForm.get('comment').value.length >= variablesService.maxCommentLength\">\n        {{ 'ASSIGN_ALIAS.FORM_ERRORS.MAX_LENGTH' | translate }}\n      </div>\n    </div>\n\n    <div class=\"alias-cost\">{{ \"ASSIGN_ALIAS.COST\" | translate : {value: alias.price | intToMoney, currency: variablesService.defaultCurrency} }}</div>\n\n    <div class=\"wrap-buttons\">\n      <button type=\"button\" class=\"blue-button\" (click)=\"assignAlias()\" [disabled]=\"!assignForm.valid || !canRegister || notEnoughMoney\">{{ 'ASSIGN_ALIAS.BUTTON_ASSIGN' | translate }}</button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"back()\">{{ 'ASSIGN_ALIAS.BUTTON_CANCEL' | translate }}</button>\n    </div>\n\n  </form>\n\n</div>\n\n"

/***/ }),

/***/ "./src/app/assign-alias/assign-alias.component.scss":
/*!**********************************************************!*\
  !*** ./src/app/assign-alias/assign-alias.component.scss ***!
  \**********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-assign {\n  margin: 2.4rem 0; }\n  .form-assign .alias-name {\n    width: 50%; }\n  .form-assign .alias-cost {\n    font-size: 1.3rem;\n    margin-top: 2rem; }\n  .form-assign .wrap-buttons {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-pack: justify;\n            justify-content: space-between;\n    margin: 2.5rem -0.7rem; }\n  .form-assign .wrap-buttons button {\n      margin: 0 0.7rem;\n      width: 15rem; }\n  .assign-alias-tooltip {\n  font-size: 1.3rem;\n  line-height: 2rem;\n  padding: 1rem 1.5rem;\n  max-width: 46rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL2Fzc2lnbi1hbGlhcy9hc3NpZ24tYWxpYXMuY29tcG9uZW50LnNjc3MiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IkFBQUE7RUFDRSxnQkFBZ0IsRUFBQTtFQURsQjtJQUlJLFVBQVUsRUFBQTtFQUpkO0lBUUksaUJBQWlCO0lBQ2pCLGdCQUFnQixFQUFBO0VBVHBCO0lBYUksb0JBQWE7SUFBYixhQUFhO0lBQ2IseUJBQThCO1lBQTlCLDhCQUE4QjtJQUM5QixzQkFBc0IsRUFBQTtFQWYxQjtNQWtCTSxnQkFBZ0I7TUFDaEIsWUFBWSxFQUFBO0VBS2xCO0VBQ0UsaUJBQWlCO0VBQ2pCLGlCQUFpQjtFQUNqQixvQkFBb0I7RUFDcEIsZ0JBQWdCLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9hc3NpZ24tYWxpYXMvYXNzaWduLWFsaWFzLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tYXNzaWduIHtcbiAgbWFyZ2luOiAyLjRyZW0gMDtcblxuICAuYWxpYXMtbmFtZSB7XG4gICAgd2lkdGg6IDUwJTtcbiAgfVxuXG4gIC5hbGlhcy1jb3N0IHtcbiAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICBtYXJnaW4tdG9wOiAycmVtO1xuICB9XG5cbiAgLndyYXAtYnV0dG9ucyB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XG4gICAgbWFyZ2luOiAyLjVyZW0gLTAuN3JlbTtcblxuICAgIGJ1dHRvbiB7XG4gICAgICBtYXJnaW46IDAgMC43cmVtO1xuICAgICAgd2lkdGg6IDE1cmVtO1xuICAgIH1cbiAgfVxufVxuXG4uYXNzaWduLWFsaWFzLXRvb2x0aXAge1xuICBmb250LXNpemU6IDEuM3JlbTtcbiAgbGluZS1oZWlnaHQ6IDJyZW07XG4gIHBhZGRpbmc6IDFyZW0gMS41cmVtO1xuICBtYXgtd2lkdGg6IDQ2cmVtO1xufVxuIl19 */"

/***/ }),

/***/ "./src/app/assign-alias/assign-alias.component.ts":
/*!********************************************************!*\
  !*** ./src/app/assign-alias/assign-alias.component.ts ***!
  \********************************************************/
/*! exports provided: AssignAliasComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "AssignAliasComponent", function() { return AssignAliasComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var _helpers_pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! ../_helpers/pipes/money-to-int.pipe */ "./src/app/_helpers/pipes/money-to-int.pipe.ts");
/* harmony import */ var _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_8__ = __webpack_require__(/*! ../_helpers/pipes/int-to-money.pipe */ "./src/app/_helpers/pipes/int-to-money.pipe.ts");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_9__ = __webpack_require__(/*! bignumber.js */ "./node_modules/bignumber.js/bignumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_9___default = /*#__PURE__*/__webpack_require__.n(bignumber_js__WEBPACK_IMPORTED_MODULE_9__);
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};










var AssignAliasComponent = /** @class */ (function () {
    function AssignAliasComponent(ngZone, location, router, backend, variablesService, modalService, moneyToInt, intToMoney) {
        var _this = this;
        this.ngZone = ngZone;
        this.location = location;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.moneyToInt = moneyToInt;
        this.intToMoney = intToMoney;
        this.assignForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            name: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [_angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required, _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].pattern(/^@?[a-z0-9\.\-]{6,25}$/)]),
            comment: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [function (g) {
                    if (g.value > _this.variablesService.maxCommentLength) {
                        return { 'maxLength': true };
                    }
                    else {
                        return null;
                    }
                }])
        });
        this.alias = {
            name: '',
            fee: this.variablesService.default_fee,
            price: new bignumber_js__WEBPACK_IMPORTED_MODULE_9___default.a(0),
            reward: '0',
            rewardOriginal: '0',
            comment: '',
            exists: false
        };
        this.canRegister = false;
        this.notEnoughMoney = false;
    }
    AssignAliasComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.wallet = this.variablesService.currentWallet;
        this.assignFormSubscription = this.assignForm.get('name').valueChanges.subscribe(function (value) {
            _this.canRegister = false;
            _this.alias.exists = false;
            var newName = value.toLowerCase().replace('@', '');
            if (!(_this.assignForm.controls['name'].errors && _this.assignForm.controls['name'].errors.hasOwnProperty('pattern')) && newName.length >= 6 && newName.length <= 25) {
                _this.backend.getAliasByName(newName, function (status) {
                    _this.ngZone.run(function () {
                        _this.alias.exists = status;
                    });
                    if (!status) {
                        _this.alias.price = new bignumber_js__WEBPACK_IMPORTED_MODULE_9___default.a(0);
                        _this.backend.getAliasCoast(newName, function (statusPrice, dataPrice) {
                            _this.ngZone.run(function () {
                                if (statusPrice) {
                                    _this.alias.price = bignumber_js__WEBPACK_IMPORTED_MODULE_9___default.a.sum(dataPrice['coast'], _this.variablesService.default_fee_big);
                                }
                                _this.notEnoughMoney = _this.alias.price.isGreaterThan(_this.wallet.unlocked_balance);
                                _this.alias.reward = _this.intToMoney.transform(_this.alias.price, false);
                                _this.alias.rewardOriginal = _this.intToMoney.transform(dataPrice['coast'], false);
                                _this.canRegister = !_this.notEnoughMoney;
                            });
                        });
                    }
                    else {
                        _this.notEnoughMoney = false;
                        _this.alias.reward = '0';
                        _this.alias.rewardOriginal = '0';
                    }
                });
            }
            else {
                _this.notEnoughMoney = false;
                _this.alias.reward = '0';
                _this.alias.rewardOriginal = '0';
            }
            _this.alias.name = newName;
        });
    };
    AssignAliasComponent.prototype.assignAlias = function () {
        var _this = this;
        var alias = this.backend.getWalletAlias(this.wallet.address);
        if (alias.hasOwnProperty('name')) {
            this.modalService.prepareModal('info', 'ASSIGN_ALIAS.ONE_ALIAS');
        }
        else {
            this.alias.comment = this.assignForm.get('comment').value;
            this.backend.registerAlias(this.wallet.wallet_id, this.alias.name, this.wallet.address, this.alias.fee, this.alias.comment, this.alias.rewardOriginal, function (status, data) {
                if (status) {
                    _this.wallet.wakeAlias = true;
                    _this.modalService.prepareModal('info', 'ASSIGN_ALIAS.REQUEST_ADD_REG');
                    _this.ngZone.run(function () {
                        _this.router.navigate(['/wallet/' + _this.wallet.wallet_id]);
                    });
                }
            });
        }
    };
    AssignAliasComponent.prototype.back = function () {
        this.location.back();
    };
    AssignAliasComponent.prototype.ngOnDestroy = function () {
        this.assignFormSubscription.unsubscribe();
    };
    AssignAliasComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-assign-alias',
            template: __webpack_require__(/*! ./assign-alias.component.html */ "./src/app/assign-alias/assign-alias.component.html"),
            styles: [__webpack_require__(/*! ./assign-alias.component.scss */ "./src/app/assign-alias/assign-alias.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _angular_common__WEBPACK_IMPORTED_MODULE_2__["Location"],
            _angular_router__WEBPACK_IMPORTED_MODULE_3__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_4__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_5__["VariablesService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_6__["ModalService"],
            _helpers_pipes_money_to_int_pipe__WEBPACK_IMPORTED_MODULE_7__["MoneyToIntPipe"],
            _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_8__["IntToMoneyPipe"]])
    ], AssignAliasComponent);
    return AssignAliasComponent;
}());



/***/ }),

/***/ "./src/app/contracts/contracts.component.html":
/*!****************************************************!*\
  !*** ./src/app/contracts/contracts.component.html ***!
  \****************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"empty-contracts\" *ngIf=\"!variablesService.currentWallet.contracts.length\">\n  <span>{{ 'CONTRACTS.EMPTY' | translate }}</span>\n</div>\n\n<div class=\"wrap-table scrolled-content\" *ngIf=\"variablesService.currentWallet.contracts.length\">\n\n  <table class=\"contracts-table\">\n    <thead>\n    <tr>\n      <th>{{ 'CONTRACTS.CONTRACTS' | translate }}</th>\n      <th>{{ 'CONTRACTS.DATE' | translate }}</th>\n      <th>{{ 'CONTRACTS.AMOUNT' | translate }}</th>\n      <th>{{ 'CONTRACTS.STATUS' | translate }}</th>\n      <th>{{ 'CONTRACTS.COMMENTS' | translate }}</th>\n    </tr>\n    </thead>\n    <tbody>\n    <tr *ngFor=\"let item of sortedArrayContracts\" [routerLink]=\"'/wallet/' + walletId + '/purchase/' + item.contract_id\">\n      <td>\n        <div class=\"contract\">\n          <i class=\"icon alert\" *ngIf=\"!item.is_new\"></i>\n          <i class=\"icon new\" *ngIf=\"item.is_new\"></i>\n          <i class=\"icon\" [class.purchase]=\"item.is_a\" [class.sell]=\"!item.is_a\"></i>\n          <span tooltip=\"{{ item.private_detailes.t }}\" placement=\"top-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{item.private_detailes.t}}</span>\n        </div>\n      </td>\n      <td>\n        <div>{{item.timestamp * 1000 | date : 'dd-MM-yyyy HH:mm'}}</div>\n      </td>\n      <td>\n        <div>{{item.private_detailes.to_pay | intToMoney}} {{variablesService.defaultCurrency}}</div>\n      </td>\n      <td>\n        <div class=\"status\" [class.error-text]=\"item.state === 4\" tooltip=\"{{item.state | contractStatusMessages : item.is_a}}\" placement=\"top\" tooltipClass=\"table-tooltip\" [delay]=\"500\">\n          {{item.state | contractStatusMessages : item.is_a}}\n        </div>\n      </td>\n      <td>\n        <div class=\"comment\" tooltip=\"{{ item.private_detailes.c }}\" placement=\"top-right\" tooltipClass=\"table-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">\n          {{item.private_detailes.c}}\n        </div>\n      </td>\n    </tr>\n    </tbody>\n  </table>\n\n</div>\n\n<div class=\"contracts-buttons\">\n  <button type=\"button\" class=\"blue-button\" [routerLink]=\"'/wallet/' + walletId + '/purchase'\">{{ 'CONTRACTS.PURCHASE_BUTTON' | translate }}</button>\n  <button type=\"button\" class=\"blue-button\" disabled>{{ 'CONTRACTS.LISTING_BUTTON' | translate }}</button>\n</div>\n"

/***/ }),

/***/ "./src/app/contracts/contracts.component.scss":
/*!****************************************************!*\
  !*** ./src/app/contracts/contracts.component.scss ***!
  \****************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.empty-contracts {\n  font-size: 1.5rem; }\n\n.wrap-table {\n  margin: -3rem -3rem 0 -3rem;\n  overflow-x: auto; }\n\n.wrap-table table tbody tr {\n    cursor: pointer;\n    outline: none !important; }\n\n.wrap-table table tbody tr .contract {\n      position: relative;\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-align: center;\n              align-items: center; }\n\n.wrap-table table tbody tr .contract .icon {\n        flex-shrink: 0; }\n\n.wrap-table table tbody tr .contract .icon.new, .wrap-table table tbody tr .contract .icon.alert {\n          position: absolute;\n          top: 0; }\n\n.wrap-table table tbody tr .contract .icon.new {\n          left: -2.3rem;\n          -webkit-mask: url('new.svg') no-repeat center;\n                  mask: url('new.svg') no-repeat center;\n          width: 1.7rem;\n          height: 1.7rem; }\n\n.wrap-table table tbody tr .contract .icon.alert {\n          top: 0.2rem;\n          left: -2.1rem;\n          -webkit-mask: url('alert.svg') no-repeat center;\n                  mask: url('alert.svg') no-repeat center;\n          width: 1.2rem;\n          height: 1.2rem; }\n\n.wrap-table table tbody tr .contract .icon.purchase, .wrap-table table tbody tr .contract .icon.sell {\n          margin-right: 1rem;\n          width: 1.5rem;\n          height: 1.5rem; }\n\n.wrap-table table tbody tr .contract .icon.purchase {\n          -webkit-mask: url('purchase.svg') no-repeat center;\n                  mask: url('purchase.svg') no-repeat center; }\n\n.wrap-table table tbody tr .contract .icon.sell {\n          -webkit-mask: url('sell.svg') no-repeat center;\n                  mask: url('sell.svg') no-repeat center; }\n\n.wrap-table table tbody tr .contract span {\n        text-overflow: ellipsis;\n        overflow: hidden; }\n\n.wrap-table table tbody tr .status, .wrap-table table tbody tr .comment {\n      display: inline-block;\n      text-overflow: ellipsis;\n      overflow: hidden;\n      max-width: 100%; }\n\n.contracts-buttons {\n  display: -webkit-box;\n  display: flex;\n  margin: 3rem 0;\n  width: 50%; }\n\n.contracts-buttons button {\n    -webkit-box-flex: 0;\n            flex: 0 1 50%;\n    margin-right: 1.5rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL2NvbnRyYWN0cy9jb250cmFjdHMuY29tcG9uZW50LnNjc3MiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IkFBQUE7RUFDRSxXQUFXLEVBQUE7O0FBR2I7RUFDRSxpQkFBaUIsRUFBQTs7QUFHbkI7RUFDRSwyQkFBMkI7RUFDM0IsZ0JBQWdCLEVBQUE7O0FBRmxCO0lBU1EsZUFBZTtJQUNmLHdCQUF3QixFQUFBOztBQVZoQztNQWFVLGtCQUFrQjtNQUNsQixvQkFBYTtNQUFiLGFBQWE7TUFDYix5QkFBbUI7Y0FBbkIsbUJBQW1CLEVBQUE7O0FBZjdCO1FBa0JZLGNBQWMsRUFBQTs7QUFsQjFCO1VBcUJjLGtCQUFrQjtVQUNsQixNQUFNLEVBQUE7O0FBdEJwQjtVQTBCYyxhQUFhO1VBQ2IsNkNBQXNEO2tCQUF0RCxxQ0FBc0Q7VUFDdEQsYUFBYTtVQUNiLGNBQWMsRUFBQTs7QUE3QjVCO1VBaUNjLFdBQVc7VUFDWCxhQUFhO1VBQ2IsK0NBQXdEO2tCQUF4RCx1Q0FBd0Q7VUFDeEQsYUFBYTtVQUNiLGNBQWMsRUFBQTs7QUFyQzVCO1VBeUNjLGtCQUFrQjtVQUNsQixhQUFhO1VBQ2IsY0FBYyxFQUFBOztBQTNDNUI7VUErQ2Msa0RBQTJEO2tCQUEzRCwwQ0FBMkQsRUFBQTs7QUEvQ3pFO1VBbURjLDhDQUF1RDtrQkFBdkQsc0NBQXVELEVBQUE7O0FBbkRyRTtRQXdEWSx1QkFBdUI7UUFDdkIsZ0JBQWdCLEVBQUE7O0FBekQ1QjtNQThEVSxxQkFBcUI7TUFDckIsdUJBQXVCO01BQ3ZCLGdCQUFnQjtNQUNoQixlQUFlLEVBQUE7O0FBT3pCO0VBQ0Usb0JBQWE7RUFBYixhQUFhO0VBQ2IsY0FBYztFQUNkLFVBQVUsRUFBQTs7QUFIWjtJQU1JLG1CQUFhO1lBQWIsYUFBYTtJQUNiLG9CQUFvQixFQUFBIiwiZmlsZSI6InNyYy9hcHAvY29udHJhY3RzL2NvbnRyYWN0cy5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcbiAgd2lkdGg6IDEwMCU7XG59XG5cbi5lbXB0eS1jb250cmFjdHMge1xuICBmb250LXNpemU6IDEuNXJlbTtcbn1cblxuLndyYXAtdGFibGUge1xuICBtYXJnaW46IC0zcmVtIC0zcmVtIDAgLTNyZW07XG4gIG92ZXJmbG93LXg6IGF1dG87XG5cbiAgdGFibGUge1xuXG4gICAgdGJvZHkge1xuXG4gICAgICB0ciB7XG4gICAgICAgIGN1cnNvcjogcG9pbnRlcjtcbiAgICAgICAgb3V0bGluZTogbm9uZSAhaW1wb3J0YW50O1xuXG4gICAgICAgIC5jb250cmFjdCB7XG4gICAgICAgICAgcG9zaXRpb246IHJlbGF0aXZlO1xuICAgICAgICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcblxuICAgICAgICAgIC5pY29uIHtcbiAgICAgICAgICAgIGZsZXgtc2hyaW5rOiAwO1xuXG4gICAgICAgICAgICAmLm5ldywgJi5hbGVydCB7XG4gICAgICAgICAgICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcbiAgICAgICAgICAgICAgdG9wOiAwO1xuICAgICAgICAgICAgfVxuXG4gICAgICAgICAgICAmLm5ldyB7XG4gICAgICAgICAgICAgIGxlZnQ6IC0yLjNyZW07XG4gICAgICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvbmV3LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgICAgICAgICAgd2lkdGg6IDEuN3JlbTtcbiAgICAgICAgICAgICAgaGVpZ2h0OiAxLjdyZW07XG4gICAgICAgICAgICB9XG5cbiAgICAgICAgICAgICYuYWxlcnQge1xuICAgICAgICAgICAgICB0b3A6IDAuMnJlbTtcbiAgICAgICAgICAgICAgbGVmdDogLTIuMXJlbTtcbiAgICAgICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9hbGVydC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICAgICAgICAgIHdpZHRoOiAxLjJyZW07XG4gICAgICAgICAgICAgIGhlaWdodDogMS4ycmVtO1xuICAgICAgICAgICAgfVxuXG4gICAgICAgICAgICAmLnB1cmNoYXNlLCAmLnNlbGwge1xuICAgICAgICAgICAgICBtYXJnaW4tcmlnaHQ6IDFyZW07XG4gICAgICAgICAgICAgIHdpZHRoOiAxLjVyZW07XG4gICAgICAgICAgICAgIGhlaWdodDogMS41cmVtO1xuICAgICAgICAgICAgfVxuXG4gICAgICAgICAgICAmLnB1cmNoYXNlIHtcbiAgICAgICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9wdXJjaGFzZS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICAgICAgICB9XG5cbiAgICAgICAgICAgICYuc2VsbCB7XG4gICAgICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvc2VsbC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICAgICAgICB9XG4gICAgICAgICAgfVxuXG4gICAgICAgICAgc3BhbiB7XG4gICAgICAgICAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcbiAgICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XG4gICAgICAgICAgfVxuICAgICAgICB9XG5cbiAgICAgICAgLnN0YXR1cywgLmNvbW1lbnQge1xuICAgICAgICAgIGRpc3BsYXk6IGlubGluZS1ibG9jaztcbiAgICAgICAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcbiAgICAgICAgICBvdmVyZmxvdzogaGlkZGVuO1xuICAgICAgICAgIG1heC13aWR0aDogMTAwJTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgIH1cbiAgfVxufVxuXG4uY29udHJhY3RzLWJ1dHRvbnMge1xuICBkaXNwbGF5OiBmbGV4O1xuICBtYXJnaW46IDNyZW0gMDtcbiAgd2lkdGg6IDUwJTtcblxuICBidXR0b24ge1xuICAgIGZsZXg6IDAgMSA1MCU7XG4gICAgbWFyZ2luLXJpZ2h0OiAxLjVyZW07XG4gIH1cbn1cbiJdfQ== */"

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
    function ContractsComponent(route, variablesService) {
        this.route = route;
        this.variablesService = variablesService;
    }
    Object.defineProperty(ContractsComponent.prototype, "sortedArrayContracts", {
        get: function () {
            return this.variablesService.currentWallet.contracts.sort(function (a, b) {
                if (a.is_new < b.is_new) {
                    return 1;
                }
                if (a.is_new > b.is_new) {
                    return -1;
                }
                if (a.timestamp < b.timestamp) {
                    return 1;
                }
                if (a.timestamp > b.timestamp) {
                    return -1;
                }
                if (a.contract_id < b.contract_id) {
                    return 1;
                }
                if (a.contract_id > b.contract_id) {
                    return -1;
                }
                return 0;
            });
        },
        enumerable: true,
        configurable: true
    });
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

module.exports = "<div class=\"content\">\n\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span [routerLink]=\"['/main']\">{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\n      <span>{{ 'BREADCRUMBS.CREATE_WALLET' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" [routerLink]=\"['/main']\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <form class=\"form-create\" [formGroup]=\"createForm\">\n\n    <div class=\"input-block\">\n      <label for=\"wallet-name\">{{ 'CREATE_WALLET.NAME' | translate }}</label>\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [attr.readonly]=\"walletSaved ? '' : null\" [maxlength]=\"variablesService.maxWalletNameLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"createForm.controls['name'].invalid && (createForm.controls['name'].dirty || createForm.controls['name'].touched)\">\n        <div *ngIf=\"createForm.controls['name'].errors['required']\">\n          {{ 'CREATE_WALLET.FORM_ERRORS.NAME_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"createForm.controls['name'].errors['duplicate']\">\n          {{ 'CREATE_WALLET.FORM_ERRORS.NAME_DUPLICATE' | translate }}\n        </div>\n      </div>\n      <div class=\"error-block\" *ngIf=\"createForm.get('name').value.length >= variablesService.maxWalletNameLength\">\n        {{ 'CREATE_WALLET.FORM_ERRORS.MAX_LENGTH' | translate }}\n      </div>\n    </div>\n\n    <div class=\"input-block\">\n      <label for=\"wallet-password\">{{ 'CREATE_WALLET.PASS' | translate }}</label>\n      <input type=\"password\" id=\"wallet-password\" formControlName=\"password\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\n    </div>\n\n    <div class=\"input-block\">\n      <label for=\"confirm-wallet-password\">{{ 'CREATE_WALLET.CONFIRM' | translate }}</label>\n      <input type=\"password\" id=\"confirm-wallet-password\" formControlName=\"confirm\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\n      <div class=\"error-block\" *ngIf=\"createForm.controls['password'].dirty && createForm.controls['confirm'].dirty && createForm.errors\">\n        <div *ngIf=\"createForm.errors['confirm_mismatch']\">\n          {{ 'CREATE_WALLET.FORM_ERRORS.CONFIRM_NOT_MATCH' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"wrap-buttons\">\n      <button type=\"button\" class=\"transparent-button\" *ngIf=\"walletSaved\" disabled><i class=\"icon\"></i>{{walletSavedName}}</button>\n      <button type=\"button\" class=\"blue-button select-button\" (click)=\"saveWallet()\" [disabled]=\"!createForm.valid\" *ngIf=\"!walletSaved\">{{ 'CREATE_WALLET.BUTTON_SELECT' | translate }}</button>\n      <button type=\"button\" class=\"blue-button create-button\" (click)=\"createWallet()\" [disabled]=\"!walletSaved\">{{ 'CREATE_WALLET.BUTTON_CREATE' | translate }}</button>\n    </div>\n\n  </form>\n\n</div>\n\n<app-progress-container [width]=\"progressWidth\" [labels]=\"['PROGRESS.ADD_WALLET', 'PROGRESS.SELECT_LOCATION', 'PROGRESS.CREATE_WALLET']\"></app-progress-container>\n"

/***/ }),

/***/ "./src/app/create-wallet/create-wallet.component.scss":
/*!************************************************************!*\
  !*** ./src/app/create-wallet/create-wallet.component.scss ***!
  \************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: relative; }\n\n.form-create {\n  margin: 2.4rem 0;\n  width: 50%; }\n\n.form-create .wrap-buttons {\n    display: -webkit-box;\n    display: flex;\n    margin: 2.5rem -0.7rem; }\n\n.form-create .wrap-buttons button {\n      margin: 0 0.7rem; }\n\n.form-create .wrap-buttons button.transparent-button {\n        flex-basis: 50%; }\n\n.form-create .wrap-buttons button.select-button {\n        flex-basis: 60%; }\n\n.form-create .wrap-buttons button.create-button {\n        -webkit-box-flex: 1;\n                flex: 1 1 50%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL2NyZWF0ZS13YWxsZXQvY3JlYXRlLXdhbGxldC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGtCQUFrQixFQUFBOztBQUdwQjtFQUNFLGdCQUFnQjtFQUNoQixVQUFVLEVBQUE7O0FBRlo7SUFLSSxvQkFBYTtJQUFiLGFBQWE7SUFDYixzQkFBc0IsRUFBQTs7QUFOMUI7TUFTTSxnQkFBZ0IsRUFBQTs7QUFUdEI7UUFZUSxlQUFlLEVBQUE7O0FBWnZCO1FBZ0JRLGVBQWUsRUFBQTs7QUFoQnZCO1FBb0JRLG1CQUFhO2dCQUFiLGFBQWEsRUFBQSIsImZpbGUiOiJzcmMvYXBwL2NyZWF0ZS13YWxsZXQvY3JlYXRlLXdhbGxldC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcbiAgcG9zaXRpb246IHJlbGF0aXZlO1xufVxuXG4uZm9ybS1jcmVhdGUge1xuICBtYXJnaW46IDIuNHJlbSAwO1xuICB3aWR0aDogNTAlO1xuXG4gIC53cmFwLWJ1dHRvbnMge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgbWFyZ2luOiAyLjVyZW0gLTAuN3JlbTtcblxuICAgIGJ1dHRvbiB7XG4gICAgICBtYXJnaW46IDAgMC43cmVtO1xuXG4gICAgICAmLnRyYW5zcGFyZW50LWJ1dHRvbiB7XG4gICAgICAgIGZsZXgtYmFzaXM6IDUwJTtcbiAgICAgIH1cblxuICAgICAgJi5zZWxlY3QtYnV0dG9uIHtcbiAgICAgICAgZmxleC1iYXNpczogNjAlO1xuICAgICAgfVxuXG4gICAgICAmLmNyZWF0ZS1idXR0b24ge1xuICAgICAgICBmbGV4OiAxIDEgNTAlO1xuICAgICAgfVxuICAgIH1cbiAgfVxufVxuIl19 */"

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
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! ../_helpers/models/wallet.model */ "./src/app/_helpers/models/wallet.model.ts");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
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
    function CreateWalletComponent(router, backend, variablesService, modalService, ngZone, translate) {
        var _this = this;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.translate = translate;
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
        this.walletSavedName = '';
        this.progressWidth = '9rem';
    }
    CreateWalletComponent.prototype.ngOnInit = function () {
    };
    CreateWalletComponent.prototype.createWallet = function () {
        var _this = this;
        this.ngZone.run(function () {
            _this.progressWidth = '100%';
            _this.router.navigate(['/seed-phrase'], { queryParams: { wallet_id: _this.wallet.id } });
        });
    };
    CreateWalletComponent.prototype.saveWallet = function () {
        var _this = this;
        if (this.createForm.valid && this.createForm.get('name').value.length <= this.variablesService.maxWalletNameLength) {
            this.backend.saveFileDialog(this.translate.instant('CREATE_WALLET.TITLE_SAVE'), '*', this.variablesService.settings.default_path, function (file_status, file_data) {
                if (file_status) {
                    _this.variablesService.settings.default_path = file_data.path.substr(0, file_data.path.lastIndexOf('/'));
                    _this.walletSavedName = file_data.path.substr(file_data.path.lastIndexOf('/') + 1, file_data.path.length - 1);
                    _this.backend.generateWallet(file_data.path, _this.createForm.get('password').value, function (generate_status, generate_data, errorCode) {
                        if (generate_status) {
                            _this.wallet.id = generate_data.wallet_id;
                            _this.variablesService.opening_wallet = new _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_6__["Wallet"](generate_data.wallet_id, _this.createForm.get('name').value, _this.createForm.get('password').value, generate_data['wi'].path, generate_data['wi'].address, generate_data['wi'].balance, generate_data['wi'].unlocked_balance, generate_data['wi'].mined_total, generate_data['wi'].tracking_hey);
                            _this.variablesService.opening_wallet.alias = _this.backend.getWalletAlias(generate_data['wi'].address);
                            _this.ngZone.run(function () {
                                _this.walletSaved = true;
                                _this.progressWidth = '50%';
                            });
                        }
                        else {
                            if (errorCode && errorCode === 'ALREADY_EXISTS') {
                                _this.modalService.prepareModal('error', 'CREATE_WALLET.ERROR_CANNOT_SAVE_TOP');
                            }
                            else {
                                _this.modalService.prepareModal('error', 'CREATE_WALLET.ERROR_CANNOT_SAVE_SYSTEM');
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
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_5__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__["ModalService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_7__["TranslateService"]])
    ], CreateWalletComponent);
    return CreateWalletComponent;
}());



/***/ }),

/***/ "./src/app/edit-alias/edit-alias.component.html":
/*!******************************************************!*\
  !*** ./src/app/edit-alias/edit-alias.component.html ***!
  \******************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\n\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span [routerLink]=\"['/wallet/' + wallet.wallet_id + '/history']\">{{ wallet.name }}</span>\n      <span>{{ 'BREADCRUMBS.EDIT_ALIAS' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <form class=\"form-edit\">\n\n    <div class=\"input-block alias-name\">\n      <label for=\"alias-name\">\n        {{ 'EDIT_ALIAS.NAME.LABEL' | translate }}\n      </label>\n      <input type=\"text\" id=\"alias-name\" [value]=\"alias.name\" placeholder=\"{{ 'EDIT_ALIAS.NAME.PLACEHOLDER' | translate }}\" readonly>\n    </div>\n\n    <div class=\"input-block textarea\">\n      <label for=\"alias-comment\">\n        {{ 'EDIT_ALIAS.COMMENT.LABEL' | translate }}\n      </label>\n      <textarea id=\"alias-comment\"\n                class=\"scrolled-content\"\n                [(ngModel)]=\"alias.comment\"\n                [ngModelOptions]=\"{standalone: true}\"\n                [maxlength]=\"variablesService.maxCommentLength\"\n                (contextmenu)=\"variablesService.onContextMenu($event)\"\n                placeholder=\"{{ 'EDIT_ALIAS.COMMENT.PLACEHOLDER' | translate }}\">\n      </textarea>\n      <div class=\"error-block\" *ngIf=\"alias.comment.length > 0 && notEnoughMoney\">\n        {{ 'EDIT_ALIAS.FORM_ERRORS.NO_MONEY' | translate }}\n      </div>\n      <div class=\"error-block\" *ngIf=\"alias.comment.length >= variablesService.maxCommentLength\">\n        {{ 'EDIT_ALIAS.FORM_ERRORS.MAX_LENGTH' | translate }}\n      </div>\n    </div>\n\n    <div class=\"alias-cost\">{{ \"EDIT_ALIAS.COST\" | translate : {value: variablesService.default_fee, currency: variablesService.defaultCurrency} }}</div>\n\n    <div class=\"wrap-buttons\">\n      <button type=\"button\" class=\"blue-button\" (click)=\"updateAlias()\" [disabled]=\"notEnoughMoney || (oldAliasComment === alias.comment) || alias.comment.length > variablesService.maxCommentLength\">{{ 'EDIT_ALIAS.BUTTON_EDIT' | translate }}</button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"back()\">{{ 'EDIT_ALIAS.BUTTON_CANCEL' | translate }}</button>\n    </div>\n\n  </form>\n\n</div>\n\n\n"

/***/ }),

/***/ "./src/app/edit-alias/edit-alias.component.scss":
/*!******************************************************!*\
  !*** ./src/app/edit-alias/edit-alias.component.scss ***!
  \******************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-edit {\n  margin: 2.4rem 0; }\n  .form-edit .alias-name {\n    width: 50%; }\n  .form-edit .alias-cost {\n    font-size: 1.3rem;\n    margin-top: 2rem; }\n  .form-edit .wrap-buttons {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-pack: justify;\n            justify-content: space-between;\n    margin: 2.5rem -0.7rem; }\n  .form-edit .wrap-buttons button {\n      margin: 0 0.7rem;\n      width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL2VkaXQtYWxpYXMvZWRpdC1hbGlhcy5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLGdCQUFnQixFQUFBO0VBRGxCO0lBSUksVUFBVSxFQUFBO0VBSmQ7SUFRSSxpQkFBaUI7SUFDakIsZ0JBQWdCLEVBQUE7RUFUcEI7SUFhSSxvQkFBYTtJQUFiLGFBQWE7SUFDYix5QkFBOEI7WUFBOUIsOEJBQThCO0lBQzlCLHNCQUFzQixFQUFBO0VBZjFCO01Ba0JNLGdCQUFnQjtNQUNoQixZQUFZLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9lZGl0LWFsaWFzL2VkaXQtYWxpYXMuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyIuZm9ybS1lZGl0IHtcbiAgbWFyZ2luOiAyLjRyZW0gMDtcblxuICAuYWxpYXMtbmFtZSB7XG4gICAgd2lkdGg6IDUwJTtcbiAgfVxuXG4gIC5hbGlhcy1jb3N0IHtcbiAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICBtYXJnaW4tdG9wOiAycmVtO1xuICB9XG5cbiAgLndyYXAtYnV0dG9ucyB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XG4gICAgbWFyZ2luOiAyLjVyZW0gLTAuN3JlbTtcblxuICAgIGJ1dHRvbiB7XG4gICAgICBtYXJnaW46IDAgMC43cmVtO1xuICAgICAgd2lkdGg6IDE1cmVtO1xuICAgIH1cbiAgfVxufVxuIl19 */"

/***/ }),

/***/ "./src/app/edit-alias/edit-alias.component.ts":
/*!****************************************************!*\
  !*** ./src/app/edit-alias/edit-alias.component.ts ***!
  \****************************************************/
/*! exports provided: EditAliasComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "EditAliasComponent", function() { return EditAliasComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var EditAliasComponent = /** @class */ (function () {
    function EditAliasComponent(location, router, backend, variablesService, modalService, ngZone) {
        this.location = location;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.requestProcessing = false;
    }
    EditAliasComponent.prototype.ngOnInit = function () {
        this.wallet = this.variablesService.currentWallet;
        var alias = this.backend.getWalletAlias(this.wallet.address);
        this.alias = {
            name: alias.name,
            address: alias.address,
            comment: alias.comment
        };
        this.oldAliasComment = alias.comment;
        this.notEnoughMoney = this.wallet.unlocked_balance.isLessThan(this.variablesService.default_fee_big);
    };
    EditAliasComponent.prototype.updateAlias = function () {
        var _this = this;
        if (this.requestProcessing || this.notEnoughMoney || this.oldAliasComment === this.alias.comment || this.alias.comment.length > this.variablesService.maxCommentLength) {
            return;
        }
        this.requestProcessing = true;
        this.backend.updateAlias(this.wallet.wallet_id, this.alias, this.variablesService.default_fee, function (status) {
            if (status) {
                _this.modalService.prepareModal('success', '');
                _this.wallet.alias['comment'] = _this.alias.comment;
                _this.ngZone.run(function () {
                    _this.router.navigate(['/wallet/' + _this.wallet.wallet_id]);
                });
            }
            _this.requestProcessing = false;
        });
    };
    EditAliasComponent.prototype.back = function () {
        this.location.back();
    };
    EditAliasComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-edit-alias',
            template: __webpack_require__(/*! ./edit-alias.component.html */ "./src/app/edit-alias/edit-alias.component.html"),
            styles: [__webpack_require__(/*! ./edit-alias.component.scss */ "./src/app/edit-alias/edit-alias.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_common__WEBPACK_IMPORTED_MODULE_1__["Location"],
            _angular_router__WEBPACK_IMPORTED_MODULE_2__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__["VariablesService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__["ModalService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], EditAliasComponent);
    return EditAliasComponent;
}());



/***/ }),

/***/ "./src/app/history/history.component.html":
/*!************************************************!*\
  !*** ./src/app/history/history.component.html ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"wrap-table\">\n\n  <table class=\"history-table\">\n    <thead>\n    <tr #head (window:resize)=\"calculateWidth()\">\n      <th>{{ 'HISTORY.STATUS' | translate }}</th>\n      <th>{{ 'HISTORY.DATE' | translate }}</th>\n      <th>{{ 'HISTORY.AMOUNT' | translate }}</th>\n      <th>{{ 'HISTORY.FEE' | translate }}</th>\n      <th>{{ 'HISTORY.ADDRESS' | translate }}</th>\n    </tr>\n    </thead>\n    <tbody>\n    <ng-container *ngFor=\"let item of variablesService.currentWallet.history\">\n      <tr (click)=\"openDetails(item.tx_hash)\" [class.locked-transaction]=\"!item.is_mining && item.unlock_time > 0\">\n        <td>\n          <div class=\"status\" [class.send]=\"!item.is_income\" [class.received]=\"item.is_income\">\n            <ng-container *ngIf=\"variablesService.height_app - item.height < 10 || item.height === 0 && item.timestamp > 0\">\n              <div class=\"confirmation\" tooltip=\"{{ 'HISTORY.STATUS_TOOLTIP' | translate : {'current': getHeight(item)/10, 'total': 10} }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\">\n                <div class=\"fill\" [style.height]=\"getHeight(item) + '%'\"></div>\n              </div>\n            </ng-container>\n            <ng-container *ngIf=\"!item.is_mining && item.unlock_time > 0\">\n              <i class=\"icon lock-transaction\" tooltip=\"{{ 'HISTORY.LOCK_TOOLTIP' | translate : {'date': item.unlock_time * 1000 | date : 'MM.dd.yy'} }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\"></i>\n            </ng-container>\n            <i class=\"icon status-transaction\"></i>\n            <span>{{ (item.is_income ? 'HISTORY.RECEIVED' : 'HISTORY.SEND') | translate }}</span>\n          </div>\n        </td>\n        <td>{{item.timestamp * 1000 | date : 'dd-MM-yyyy HH:mm'}}</td>\n        <td>\n          <span *ngIf=\"item.sortAmount && item.sortAmount.toString() !== '0'\">{{item.sortAmount | intToMoney}} {{variablesService.defaultCurrency}}</span>\n        </td>\n        <td>\n          <span *ngIf=\"item.sortFee && item.sortFee.toString() !== '0'\">{{item.sortFee | intToMoney}} {{variablesService.defaultCurrency}}</span>\n        </td>\n        <td class=\"remote-address\">\n          <span *ngIf=\"!(item.tx_type === 0 && item.remote_addresses && item.remote_addresses[0])\">{{item | historyTypeMessages}}</span>\n          <span *ngIf=\"item.tx_type === 0 && item.remote_addresses && item.remote_addresses[0]\" (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, item.remote_addresses[0])\">{{item.remote_addresses[0]}}</span>\n        </td>\n      </tr>\n      <tr class=\"transaction-details\" [class.open]=\"item.tx_hash === openedDetails\">\n        <td colspan=\"5\">\n          <ng-container *ngIf=\"item.tx_hash === openedDetails\">\n            <app-transaction-details [transaction]=\"item\" [sizes]=\"calculatedWidth\"></app-transaction-details>\n          </ng-container>\n        </td>\n      </tr>\n    </ng-container>\n    </tbody>\n  </table>\n\n</div>\n"

/***/ }),

/***/ "./src/app/history/history.component.scss":
/*!************************************************!*\
  !*** ./src/app/history/history.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.wrap-table {\n  margin: -3rem; }\n\n.wrap-table table tbody tr td {\n    min-width: 10rem; }\n\n.wrap-table table tbody tr .status {\n    position: relative;\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center; }\n\n.wrap-table table tbody tr .status .confirmation {\n      position: absolute;\n      top: 50%;\n      left: -2rem;\n      -webkit-transform: translateY(-50%);\n              transform: translateY(-50%);\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-align: end;\n              align-items: flex-end;\n      width: 0.7rem;\n      height: 1.5rem; }\n\n.wrap-table table tbody tr .status .confirmation .fill {\n        width: 100%; }\n\n.wrap-table table tbody tr .status .lock-transaction {\n      position: absolute;\n      top: 50%;\n      left: -2rem;\n      -webkit-transform: translateY(-50%);\n              transform: translateY(-50%);\n      -webkit-mask: url('lock-transaction.svg') no-repeat center;\n              mask: url('lock-transaction.svg') no-repeat center;\n      width: 1.2rem;\n      height: 1.2rem; }\n\n.wrap-table table tbody tr .status .status-transaction {\n      margin-right: 1rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.wrap-table table tbody tr .status.send .status-transaction {\n      -webkit-mask: url('send.svg') no-repeat center;\n              mask: url('send.svg') no-repeat center; }\n\n.wrap-table table tbody tr .status.received .status-transaction {\n      -webkit-mask: url('receive.svg') no-repeat center;\n              mask: url('receive.svg') no-repeat center; }\n\n.wrap-table table tbody tr .remote-address {\n    overflow: hidden;\n    text-overflow: ellipsis;\n    max-width: 25vw; }\n\n.wrap-table table tbody tr:not(.transaction-details) {\n    cursor: pointer; }\n\n.wrap-table table tbody tr.transaction-details {\n    -webkit-transition: 0.5s height linear, 0s font-size;\n    transition: 0.5s height linear, 0s font-size;\n    -webkit-transition-delay: 0s, 0.5s;\n            transition-delay: 0s, 0.5s;\n    height: 0; }\n\n.wrap-table table tbody tr.transaction-details.open {\n      height: 13.2rem; }\n\n.wrap-table table tbody tr.transaction-details td {\n      position: relative;\n      overflow: hidden;\n      line-height: inherit;\n      padding-top: 0;\n      padding-bottom: 0; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL2hpc3RvcnkvaGlzdG9yeS5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLFdBQVcsRUFBQTs7QUFHYjtFQUNFLGFBQWEsRUFBQTs7QUFEZjtJQVVVLGdCQUFnQixFQUFBOztBQVYxQjtJQWNVLGtCQUFrQjtJQUNsQixvQkFBYTtJQUFiLGFBQWE7SUFDYix5QkFBbUI7WUFBbkIsbUJBQW1CLEVBQUE7O0FBaEI3QjtNQW1CWSxrQkFBa0I7TUFDbEIsUUFBUTtNQUNSLFdBQVc7TUFDWCxtQ0FBMkI7Y0FBM0IsMkJBQTJCO01BQzNCLG9CQUFhO01BQWIsYUFBYTtNQUNiLHNCQUFxQjtjQUFyQixxQkFBcUI7TUFDckIsYUFBYTtNQUNiLGNBQWMsRUFBQTs7QUExQjFCO1FBNkJjLFdBQVcsRUFBQTs7QUE3QnpCO01Ba0NZLGtCQUFrQjtNQUNsQixRQUFRO01BQ1IsV0FBVztNQUNYLG1DQUEyQjtjQUEzQiwyQkFBMkI7TUFDM0IsMERBQW1FO2NBQW5FLGtEQUFtRTtNQUNuRSxhQUFhO01BQ2IsY0FBYyxFQUFBOztBQXhDMUI7TUE0Q1ksa0JBQWtCO01BQ2xCLGFBQWE7TUFDYixjQUFjLEVBQUE7O0FBOUMxQjtNQW9EYyw4Q0FBdUQ7Y0FBdkQsc0NBQXVELEVBQUE7O0FBcERyRTtNQTJEYyxpREFBMEQ7Y0FBMUQseUNBQTBELEVBQUE7O0FBM0R4RTtJQWlFVSxnQkFBZ0I7SUFDaEIsdUJBQXVCO0lBQ3ZCLGVBQWUsRUFBQTs7QUFuRXpCO0lBdUVVLGVBQWUsRUFBQTs7QUF2RXpCO0lBMkVVLG9EQUFvRDtJQUNwRCw0Q0FBNEM7SUFDNUMsa0NBQTBCO1lBQTFCLDBCQUEwQjtJQUMxQixTQUFTLEVBQUE7O0FBOUVuQjtNQWlGWSxlQUFlLEVBQUE7O0FBakYzQjtNQXFGWSxrQkFBa0I7TUFDbEIsZ0JBQWdCO01BQ2hCLG9CQUFvQjtNQUNwQixjQUFjO01BQ2QsaUJBQWlCLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9oaXN0b3J5L2hpc3RvcnkuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XG4gIHdpZHRoOiAxMDAlO1xufVxuXG4ud3JhcC10YWJsZSB7XG4gIG1hcmdpbjogLTNyZW07XG5cbiAgdGFibGUge1xuXG4gICAgdGJvZHkge1xuXG4gICAgICB0ciB7XG5cbiAgICAgICAgdGQge1xuICAgICAgICAgIG1pbi13aWR0aDogMTByZW07XG4gICAgICAgIH1cblxuICAgICAgICAuc3RhdHVzIHtcbiAgICAgICAgICBwb3NpdGlvbjogcmVsYXRpdmU7XG4gICAgICAgICAgZGlzcGxheTogZmxleDtcbiAgICAgICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuXG4gICAgICAgICAgLmNvbmZpcm1hdGlvbiB7XG4gICAgICAgICAgICBwb3NpdGlvbjogYWJzb2x1dGU7XG4gICAgICAgICAgICB0b3A6IDUwJTtcbiAgICAgICAgICAgIGxlZnQ6IC0ycmVtO1xuICAgICAgICAgICAgdHJhbnNmb3JtOiB0cmFuc2xhdGVZKC01MCUpO1xuICAgICAgICAgICAgZGlzcGxheTogZmxleDtcbiAgICAgICAgICAgIGFsaWduLWl0ZW1zOiBmbGV4LWVuZDtcbiAgICAgICAgICAgIHdpZHRoOiAwLjdyZW07XG4gICAgICAgICAgICBoZWlnaHQ6IDEuNXJlbTtcblxuICAgICAgICAgICAgLmZpbGwge1xuICAgICAgICAgICAgICB3aWR0aDogMTAwJTtcbiAgICAgICAgICAgIH1cbiAgICAgICAgICB9XG5cbiAgICAgICAgICAubG9jay10cmFuc2FjdGlvbiB7XG4gICAgICAgICAgICBwb3NpdGlvbjogYWJzb2x1dGU7XG4gICAgICAgICAgICB0b3A6IDUwJTtcbiAgICAgICAgICAgIGxlZnQ6IC0ycmVtO1xuICAgICAgICAgICAgdHJhbnNmb3JtOiB0cmFuc2xhdGVZKC01MCUpO1xuICAgICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9sb2NrLXRyYW5zYWN0aW9uLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgICAgICAgIHdpZHRoOiAxLjJyZW07XG4gICAgICAgICAgICBoZWlnaHQ6IDEuMnJlbTtcbiAgICAgICAgICB9XG5cbiAgICAgICAgICAuc3RhdHVzLXRyYW5zYWN0aW9uIHtcbiAgICAgICAgICAgIG1hcmdpbi1yaWdodDogMXJlbTtcbiAgICAgICAgICAgIHdpZHRoOiAxLjdyZW07XG4gICAgICAgICAgICBoZWlnaHQ6IDEuN3JlbTtcbiAgICAgICAgICB9XG5cbiAgICAgICAgICAmLnNlbmQgIHtcblxuICAgICAgICAgICAgLnN0YXR1cy10cmFuc2FjdGlvbiB7XG4gICAgICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvc2VuZC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICAgICAgICB9XG4gICAgICAgICAgfVxuXG4gICAgICAgICAgJi5yZWNlaXZlZCB7XG5cbiAgICAgICAgICAgIC5zdGF0dXMtdHJhbnNhY3Rpb24ge1xuICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3JlY2VpdmUuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xuICAgICAgICAgICAgfVxuICAgICAgICAgIH1cbiAgICAgICAgfVxuXG4gICAgICAgIC5yZW1vdGUtYWRkcmVzcyB7XG4gICAgICAgICAgb3ZlcmZsb3c6IGhpZGRlbjtcbiAgICAgICAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcbiAgICAgICAgICBtYXgtd2lkdGg6IDI1dnc7XG4gICAgICAgIH1cblxuICAgICAgICAmOm5vdCgudHJhbnNhY3Rpb24tZGV0YWlscykge1xuICAgICAgICAgIGN1cnNvcjogcG9pbnRlcjtcbiAgICAgICAgfVxuXG4gICAgICAgICYudHJhbnNhY3Rpb24tZGV0YWlscyB7XG4gICAgICAgICAgLXdlYmtpdC10cmFuc2l0aW9uOiAwLjVzIGhlaWdodCBsaW5lYXIsIDBzIGZvbnQtc2l6ZTtcbiAgICAgICAgICB0cmFuc2l0aW9uOiAwLjVzIGhlaWdodCBsaW5lYXIsIDBzIGZvbnQtc2l6ZTtcbiAgICAgICAgICB0cmFuc2l0aW9uLWRlbGF5OiAwcywgMC41cztcbiAgICAgICAgICBoZWlnaHQ6IDA7XG5cbiAgICAgICAgICAmLm9wZW4ge1xuICAgICAgICAgICAgaGVpZ2h0OiAxMy4ycmVtO1xuICAgICAgICAgIH1cblxuICAgICAgICAgIHRkIHtcbiAgICAgICAgICAgIHBvc2l0aW9uOiByZWxhdGl2ZTtcbiAgICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XG4gICAgICAgICAgICBsaW5lLWhlaWdodDogaW5oZXJpdDtcbiAgICAgICAgICAgIHBhZGRpbmctdG9wOiAwO1xuICAgICAgICAgICAgcGFkZGluZy1ib3R0b206IDA7XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICB9XG4gICAgfVxuICB9XG59XG4iXX0= */"

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
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
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
    function HistoryComponent(route, variablesService) {
        this.route = route;
        this.variablesService = variablesService;
        this.openedDetails = false;
        this.calculatedWidth = [];
    }
    HistoryComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function () {
            _this.openedDetails = false;
        });
    };
    HistoryComponent.prototype.ngAfterViewChecked = function () {
        this.calculateWidth();
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
    HistoryComponent.prototype.openDetails = function (tx_hash) {
        if (tx_hash === this.openedDetails) {
            this.openedDetails = false;
        }
        else {
            this.openedDetails = tx_hash;
        }
    };
    HistoryComponent.prototype.calculateWidth = function () {
        this.calculatedWidth = [];
        this.calculatedWidth.push(this.head.nativeElement.childNodes[0].clientWidth);
        this.calculatedWidth.push(this.head.nativeElement.childNodes[1].clientWidth + this.head.nativeElement.childNodes[2].clientWidth);
        this.calculatedWidth.push(this.head.nativeElement.childNodes[3].clientWidth);
        this.calculatedWidth.push(this.head.nativeElement.childNodes[4].clientWidth);
    };
    HistoryComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["ViewChild"])('head'),
        __metadata("design:type", _angular_core__WEBPACK_IMPORTED_MODULE_0__["ElementRef"])
    ], HistoryComponent.prototype, "head", void 0);
    HistoryComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-history',
            template: __webpack_require__(/*! ./history.component.html */ "./src/app/history/history.component.html"),
            styles: [__webpack_require__(/*! ./history.component.scss */ "./src/app/history/history.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_2__["ActivatedRoute"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"]])
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

module.exports = "<div class=\"content\">\n\n  <div class=\"wrap-login\">\n\n    <div class=\"logo\"></div>\n\n    <form *ngIf=\"type === 'reg'\" class=\"form-login\" [formGroup]=\"regForm\" (ngSubmit)=\"onSubmitCreatePass()\">\n\n      <div class=\"input-block\">\n        <label for=\"master-pass\">{{ 'LOGIN.SETUP_MASTER_PASS' | translate }}</label>\n        <input type=\"password\" id=\"master-pass\" formControlName=\"password\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\n      </div>\n\n      <div class=\"input-block\">\n        <label for=\"confirm-pass\">{{ 'LOGIN.SETUP_CONFIRM_PASS' | translate }}</label>\n        <input type=\"password\" id=\"confirm-pass\" formControlName=\"confirmation\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\n        <div class=\"error-block\" *ngIf=\"regForm.controls['password'].dirty && regForm.controls['confirmation'].dirty && regForm.errors\">\n          <div *ngIf=\"regForm.errors['mismatch']\">\n            {{ 'LOGIN.FORM_ERRORS.MISMATCH' | translate }}\n          </div>\n        </div>\n      </div>\n\n      <div class=\"wrap-button\">\n        <button type=\"submit\" class=\"blue-button\" [disabled]=\"!regForm.controls['password'].value.length || !regForm.controls['confirmation'].value.length || (regForm.errors && regForm.errors['mismatch'])\">{{ 'LOGIN.BUTTON_NEXT' | translate }}</button>\n        <button type=\"button\" class=\"blue-button\" (click)=\"onSkipCreatePass()\" [disabled]=\"regForm.controls['password'].value.length || regForm.controls['confirmation'].value.length\">{{ 'LOGIN.BUTTON_SKIP' | translate }}</button>\n      </div>\n\n    </form>\n\n    <form *ngIf=\"type !== 'reg'\" class=\"form-login\" [formGroup]=\"authForm\" (ngSubmit)=\"onSubmitAuthPass()\">\n\n      <div class=\"input-block\">\n        <label for=\"master-pass-login\">{{ 'LOGIN.MASTER_PASS' | translate }}</label>\n        <input type=\"password\" id=\"master-pass-login\" formControlName=\"password\" autofocus (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\n      </div>\n\n      <button type=\"submit\" class=\"blue-button\">{{ 'LOGIN.BUTTON_NEXT' | translate }}</button>\n\n    </form>\n\n  </div>\n\n</div>\n"

/***/ }),

/***/ "./src/app/login/login.component.scss":
/*!********************************************!*\
  !*** ./src/app/login/login.component.scss ***!
  \********************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: fixed;\n  top: 0;\n  left: 0;\n  width: 100%;\n  height: 100%; }\n  :host .content {\n    display: -webkit-box;\n    display: flex; }\n  :host .content .wrap-login {\n      margin: auto;\n      width: 100%;\n      max-width: 40rem; }\n  :host .content .wrap-login .logo {\n        background: url('logo.svg') no-repeat center;\n        width: 100%;\n        height: 15rem; }\n  :host .content .wrap-login .form-login {\n        display: -webkit-box;\n        display: flex;\n        -webkit-box-orient: vertical;\n        -webkit-box-direction: normal;\n                flex-direction: column; }\n  :host .content .wrap-login .form-login .wrap-button {\n          display: -webkit-box;\n          display: flex;\n          -webkit-box-align: center;\n                  align-items: center;\n          -webkit-box-pack: justify;\n                  justify-content: space-between; }\n  :host .content .wrap-login .form-login .wrap-button button {\n            margin: 2.5rem 0; }\n  :host .content .wrap-login .form-login button {\n          margin: 2.5rem auto;\n          width: 100%;\n          max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL2xvZ2luL2xvZ2luLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZUFBZTtFQUNmLE1BQU07RUFDTixPQUFPO0VBQ1AsV0FBVztFQUNYLFlBQVksRUFBQTtFQUxkO0lBUUksb0JBQWE7SUFBYixhQUFhLEVBQUE7RUFSakI7TUFXTSxZQUFZO01BQ1osV0FBVztNQUNYLGdCQUFnQixFQUFBO0VBYnRCO1FBZ0JRLDRDQUE2RDtRQUM3RCxXQUFXO1FBQ1gsYUFBYSxFQUFBO0VBbEJyQjtRQXNCUSxvQkFBYTtRQUFiLGFBQWE7UUFDYiw0QkFBc0I7UUFBdEIsNkJBQXNCO2dCQUF0QixzQkFBc0IsRUFBQTtFQXZCOUI7VUEwQlUsb0JBQWE7VUFBYixhQUFhO1VBQ2IseUJBQW1CO2tCQUFuQixtQkFBbUI7VUFDbkIseUJBQThCO2tCQUE5Qiw4QkFBOEIsRUFBQTtFQTVCeEM7WUErQlksZ0JBQWdCLEVBQUE7RUEvQjVCO1VBb0NVLG1CQUFtQjtVQUNuQixXQUFXO1VBQ1gsZ0JBQWdCLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9sb2dpbi9sb2dpbi5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcbiAgcG9zaXRpb246IGZpeGVkO1xuICB0b3A6IDA7XG4gIGxlZnQ6IDA7XG4gIHdpZHRoOiAxMDAlO1xuICBoZWlnaHQ6IDEwMCU7XG5cbiAgLmNvbnRlbnQge1xuICAgIGRpc3BsYXk6IGZsZXg7XG5cbiAgICAud3JhcC1sb2dpbiB7XG4gICAgICBtYXJnaW46IGF1dG87XG4gICAgICB3aWR0aDogMTAwJTtcbiAgICAgIG1heC13aWR0aDogNDByZW07XG5cbiAgICAgIC5sb2dvIHtcbiAgICAgICAgYmFja2dyb3VuZDogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9sb2dvLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgICAgd2lkdGg6IDEwMCU7XG4gICAgICAgIGhlaWdodDogMTVyZW07XG4gICAgICB9XG5cbiAgICAgIC5mb3JtLWxvZ2luIHtcbiAgICAgICAgZGlzcGxheTogZmxleDtcbiAgICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcblxuICAgICAgICAud3JhcC1idXR0b24ge1xuICAgICAgICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICAgICAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XG5cbiAgICAgICAgICBidXR0b24ge1xuICAgICAgICAgICAgbWFyZ2luOiAyLjVyZW0gMDtcbiAgICAgICAgICB9XG4gICAgICAgIH1cblxuICAgICAgICBidXR0b24ge1xuICAgICAgICAgIG1hcmdpbjogMi41cmVtIGF1dG87XG4gICAgICAgICAgd2lkdGg6IDEwMCU7XG4gICAgICAgICAgbWF4LXdpZHRoOiAxNXJlbTtcbiAgICAgICAgfVxuICAgICAgfVxuICAgIH1cbiAgfVxufVxuIl19 */"

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
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! ../_helpers/models/wallet.model */ "./src/app/_helpers/models/wallet.model.ts");
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
    function LoginComponent(route, router, backend, variablesService, modalService, ngZone) {
        this.route = route;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.regForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            password: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](''),
            confirmation: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('')
        }, function (g) {
            return g.get('password').value === g.get('confirmation').value ? null : { 'mismatch': true };
        });
        this.authForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            password: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('')
        });
        this.type = 'reg';
    }
    LoginComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.queryRouting = this.route.queryParams.subscribe(function (params) {
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
                    _this.variablesService.appLogin = true;
                    _this.variablesService.startCountdown();
                    _this.ngZone.run(function () {
                        _this.router.navigate(['/']);
                    });
                }
                else {
                    console.log(data['error_code']);
                }
            });
        }
    };
    LoginComponent.prototype.onSkipCreatePass = function () {
        var _this = this;
        this.variablesService.appPass = '';
        this.ngZone.run(function () {
            _this.variablesService.appLogin = true;
            _this.router.navigate(['/']);
        });
    };
    LoginComponent.prototype.onSubmitAuthPass = function () {
        var _this = this;
        if (this.authForm.valid) {
            var appPass_1 = this.authForm.get('password').value;
            this.backend.getSecureAppData({ pass: appPass_1 }, function (status, data) {
                if (!data.error_code) {
                    _this.variablesService.appLogin = true;
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
                            _this.backend.openWallet(wallet.path, wallet.pass, true, function (open_status, open_data, open_error) {
                                if (open_status || open_error === 'FILE_RESTORED') {
                                    openWallets_1++;
                                    _this.ngZone.run(function () {
                                        var new_wallet = new _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_6__["Wallet"](open_data.wallet_id, wallet.name, wallet.pass, open_data['wi'].path, open_data['wi'].address, open_data['wi'].balance, open_data['wi'].unlocked_balance, open_data['wi'].mined_total, open_data['wi'].tracking_hey);
                                        new_wallet.alias = _this.backend.getWalletAlias(new_wallet.address);
                                        if (wallet.staking) {
                                            new_wallet.staking = true;
                                            _this.backend.startPosMining(new_wallet.wallet_id);
                                        }
                                        else {
                                            new_wallet.staking = false;
                                        }
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
                                    _this.backend.runWallet(open_data.wallet_id, function (run_status) {
                                        if (run_status) {
                                            runWallets_1++;
                                        }
                                        else {
                                            if (wallet_index === data.length - 1 && runWallets_1 === 0) {
                                                _this.ngZone.run(function () {
                                                    _this.router.navigate(['/']);
                                                });
                                            }
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
    LoginComponent.prototype.ngOnDestroy = function () {
        this.queryRouting.unsubscribe();
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
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__["ModalService"],
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

module.exports = "<div class=\"content\">\n\n  <div class=\"head\" *ngIf=\"variablesService.wallets.length > 0\">\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <div class=\"add-wallet\">\n    <h3 class=\"add-wallet-title\">{{ 'MAIN.TITLE' | translate }}</h3>\n    <div class=\"add-wallet-buttons\">\n      <button type=\"button\" class=\"blue-button\" [routerLink]=\"['/create']\">{{ 'MAIN.BUTTON_NEW_WALLET' | translate }}</button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"openWallet()\">{{ 'MAIN.BUTTON_OPEN_WALLET' | translate }}</button>\n      <button type=\"button\" class=\"blue-button\" [routerLink]=\"['/restore']\">{{ 'MAIN.BUTTON_RESTORE_BACKUP' | translate }}</button>\n    </div>\n    <div class=\"add-wallet-help\" (click)=\"openInBrowser()\">\n      <i class=\"icon\"></i><span>{{ 'MAIN.HELP' | translate }}</span>\n    </div>\n  </div>\n\n</div>\n"

/***/ }),

/***/ "./src/app/main/main.component.scss":
/*!******************************************!*\
  !*** ./src/app/main/main.component.scss ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  -webkit-box-flex: 1;\n          flex: 1 0 auto;\n  padding: 3rem; }\n\n.content {\n  padding: 3rem;\n  min-height: 100%; }\n\n.content .head {\n    -webkit-box-pack: end;\n            justify-content: flex-end; }\n\n.add-wallet .add-wallet-title {\n  margin-bottom: 1rem; }\n\n.add-wallet .add-wallet-buttons {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  -webkit-box-pack: justify;\n          justify-content: space-between;\n  margin: 0 -0.5rem;\n  padding: 1.5rem 0; }\n\n.add-wallet .add-wallet-buttons button {\n    -webkit-box-flex: 1;\n            flex: 1 0 auto;\n    margin: 0 0.5rem; }\n\n.add-wallet .add-wallet-help {\n  display: -webkit-box;\n  display: flex;\n  cursor: pointer;\n  font-size: 1.3rem;\n  line-height: 1.4rem; }\n\n.add-wallet .add-wallet-help .icon {\n    -webkit-mask: url('howto.svg') no-repeat center;\n            mask: url('howto.svg') no-repeat center;\n    margin-right: 0.8rem;\n    width: 1.4rem;\n    height: 1.4rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL21haW4vbWFpbi5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLG1CQUFjO1VBQWQsY0FBYztFQUNkLGFBQWEsRUFBQTs7QUFHZjtFQUNFLGFBQWE7RUFDYixnQkFBZ0IsRUFBQTs7QUFGbEI7SUFLSSxxQkFBeUI7WUFBekIseUJBQXlCLEVBQUE7O0FBSTdCO0VBR0ksbUJBQW1CLEVBQUE7O0FBSHZCO0VBT0ksb0JBQWE7RUFBYixhQUFhO0VBQ2IseUJBQW1CO1VBQW5CLG1CQUFtQjtFQUNuQix5QkFBOEI7VUFBOUIsOEJBQThCO0VBQzlCLGlCQUFpQjtFQUNqQixpQkFBaUIsRUFBQTs7QUFYckI7SUFjTSxtQkFBYztZQUFkLGNBQWM7SUFDZCxnQkFBZ0IsRUFBQTs7QUFmdEI7RUFvQkksb0JBQWE7RUFBYixhQUFhO0VBQ2IsZUFBZTtFQUNmLGlCQUFpQjtFQUNqQixtQkFBbUIsRUFBQTs7QUF2QnZCO0lBMEJNLCtDQUF3RDtZQUF4RCx1Q0FBd0Q7SUFDeEQsb0JBQW9CO0lBQ3BCLGFBQWE7SUFDYixjQUFjLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9tYWluL21haW4uY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XG4gIGZsZXg6IDEgMCBhdXRvO1xuICBwYWRkaW5nOiAzcmVtO1xufVxuXG4uY29udGVudCB7XG4gIHBhZGRpbmc6IDNyZW07XG4gIG1pbi1oZWlnaHQ6IDEwMCU7XG5cbiAgLmhlYWQge1xuICAgIGp1c3RpZnktY29udGVudDogZmxleC1lbmQ7XG4gIH1cbn1cblxuLmFkZC13YWxsZXQge1xuXG4gIC5hZGQtd2FsbGV0LXRpdGxlIHtcbiAgICBtYXJnaW4tYm90dG9tOiAxcmVtO1xuICB9XG5cbiAgLmFkZC13YWxsZXQtYnV0dG9ucyB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcbiAgICBtYXJnaW46IDAgLTAuNXJlbTtcbiAgICBwYWRkaW5nOiAxLjVyZW0gMDtcblxuICAgIGJ1dHRvbiB7XG4gICAgICBmbGV4OiAxIDAgYXV0bztcbiAgICAgIG1hcmdpbjogMCAwLjVyZW07XG4gICAgfVxuICB9XG5cbiAgLmFkZC13YWxsZXQtaGVscCB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBjdXJzb3I6IHBvaW50ZXI7XG4gICAgZm9udC1zaXplOiAxLjNyZW07XG4gICAgbGluZS1oZWlnaHQ6IDEuNHJlbTtcblxuICAgIC5pY29uIHtcbiAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvaG93dG8uc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xuICAgICAgbWFyZ2luLXJpZ2h0OiAwLjhyZW07XG4gICAgICB3aWR0aDogMS40cmVtO1xuICAgICAgaGVpZ2h0OiAxLjRyZW07XG4gICAgfVxuICB9XG59XG4iXX0= */"

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
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
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
    function MainComponent(router, location, backend, variablesService, ngZone, translate) {
        this.router = router;
        this.location = location;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.translate = translate;
    }
    MainComponent.prototype.ngOnInit = function () {
    };
    MainComponent.prototype.openWallet = function () {
        var _this = this;
        this.backend.openFileDialog(this.translate.instant('MAIN.CHOOSE_PATH'), '*', this.variablesService.settings.default_path, function (file_status, file_data) {
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
    MainComponent.prototype.openInBrowser = function () {
        this.backend.openUrlInBrowser('docs.zano.org/docs/getting-started-1#section-create-new-wallet');
    };
    MainComponent.prototype.back = function () {
        this.location.back();
    };
    MainComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-main',
            template: __webpack_require__(/*! ./main.component.html */ "./src/app/main/main.component.html"),
            styles: [__webpack_require__(/*! ./main.component.scss */ "./src/app/main/main.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_4__["Router"],
            _angular_common__WEBPACK_IMPORTED_MODULE_1__["Location"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_5__["TranslateService"]])
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

module.exports = "<div class=\"wrap-table\">\n\n  <table class=\"messages-table\">\n    <thead>\n    <tr>\n      <th>{{ 'MESSAGES.ADDRESS' | translate }}</th>\n      <th>{{ 'MESSAGES.MESSAGE' | translate }}</th>\n    </tr>\n    </thead>\n    <tbody>\n    <tr *ngFor=\"let message of messages\" [routerLink]=\"[message.address]\">\n      <td>\n        <span>{{message.address}}</span>\n        <i class=\"icon\" *ngIf=\"message.is_new\"></i>\n      </td>\n      <td>\n        <span>{{message.message}}</span>\n      </td>\n    </tr>\n    </tbody>\n  </table>\n\n</div>\n"

/***/ }),

/***/ "./src/app/messages/messages.component.scss":
/*!**************************************************!*\
  !*** ./src/app/messages/messages.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.wrap-table {\n  margin: -3rem; }\n\n.wrap-table table tbody tr td:first-child {\n    position: relative;\n    padding-right: 5rem;\n    width: 18rem; }\n\n.wrap-table table tbody tr td:first-child span {\n      display: block;\n      line-height: 3.5rem;\n      max-width: 10rem; }\n\n.wrap-table table tbody tr td:first-child .icon {\n      position: absolute;\n      top: 50%;\n      right: 1rem;\n      -webkit-transform: translateY(-50%);\n              transform: translateY(-50%);\n      display: block;\n      -webkit-mask: url('alert.svg') no-repeat 0;\n              mask: url('alert.svg') no-repeat 0;\n      width: 1.2rem;\n      height: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL21lc3NhZ2VzL21lc3NhZ2VzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsV0FBVyxFQUFBOztBQUdiO0VBQ0UsYUFBYSxFQUFBOztBQURmO0lBWVksa0JBQWtCO0lBQ2xCLG1CQUFtQjtJQUNuQixZQUFZLEVBQUE7O0FBZHhCO01BaUJjLGNBQWM7TUFDZCxtQkFBbUI7TUFDbkIsZ0JBQWdCLEVBQUE7O0FBbkI5QjtNQXVCYyxrQkFBa0I7TUFDbEIsUUFBUTtNQUNSLFdBQVc7TUFDWCxtQ0FBMkI7Y0FBM0IsMkJBQTJCO01BQzNCLGNBQWM7TUFDZCwwQ0FBbUQ7Y0FBbkQsa0NBQW1EO01BQ25ELGFBQWE7TUFDYixjQUFjLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9tZXNzYWdlcy9tZXNzYWdlcy5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcbiAgd2lkdGg6IDEwMCU7XG59XG5cbi53cmFwLXRhYmxlIHtcbiAgbWFyZ2luOiAtM3JlbTtcblxuICB0YWJsZSB7XG5cbiAgICB0Ym9keSB7XG5cbiAgICAgIHRyIHtcblxuICAgICAgICB0ZCB7XG5cbiAgICAgICAgICAmOmZpcnN0LWNoaWxkIHtcbiAgICAgICAgICAgIHBvc2l0aW9uOiByZWxhdGl2ZTtcbiAgICAgICAgICAgIHBhZGRpbmctcmlnaHQ6IDVyZW07XG4gICAgICAgICAgICB3aWR0aDogMThyZW07XG5cbiAgICAgICAgICAgIHNwYW4ge1xuICAgICAgICAgICAgICBkaXNwbGF5OiBibG9jaztcbiAgICAgICAgICAgICAgbGluZS1oZWlnaHQ6IDMuNXJlbTtcbiAgICAgICAgICAgICAgbWF4LXdpZHRoOiAxMHJlbTtcbiAgICAgICAgICAgIH1cblxuICAgICAgICAgICAgLmljb24ge1xuICAgICAgICAgICAgICBwb3NpdGlvbjogYWJzb2x1dGU7XG4gICAgICAgICAgICAgIHRvcDogNTAlO1xuICAgICAgICAgICAgICByaWdodDogMXJlbTtcbiAgICAgICAgICAgICAgdHJhbnNmb3JtOiB0cmFuc2xhdGVZKC01MCUpO1xuICAgICAgICAgICAgICBkaXNwbGF5OiBibG9jaztcbiAgICAgICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9hbGVydC5zdmcpIG5vLXJlcGVhdCAwO1xuICAgICAgICAgICAgICB3aWR0aDogMS4ycmVtO1xuICAgICAgICAgICAgICBoZWlnaHQ6IDEuMnJlbTtcbiAgICAgICAgICAgIH1cbiAgICAgICAgICB9XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9XG4gIH1cbn1cbiJdfQ== */"

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

/***/ "./src/app/open-wallet-modal/open-wallet-modal.component.html":
/*!********************************************************************!*\
  !*** ./src/app/open-wallet-modal/open-wallet-modal.component.html ***!
  \********************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"modal\">\n  <h3 class=\"title\">{{ 'OPEN_WALLET.MODAL.TITLE' | translate }}</h3>\n  <form class=\"open-form\" (ngSubmit)=\"openWallet()\">\n    <div class=\"wallet-path\">{{ wallet.name }}</div>\n    <div class=\"wallet-path\">{{ wallet.path }}</div>\n    <div class=\"input-block\" *ngIf=\"!wallet.notFound && !wallet.emptyPass\">\n      <label for=\"password\">{{ 'OPEN_WALLET.MODAL.LABEL' | translate }}</label>\n      <input type=\"password\" id=\"password\" name=\"password\" [(ngModel)]=\"wallet.pass\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\"/>\n    </div>\n    <div class=\"error-block\" *ngIf=\"wallet.notFound\">\n      {{ 'OPEN_WALLET.MODAL.NOT_FOUND' | translate }}\n    </div>\n    <div class=\"wrap-button\">\n      <button type=\"submit\" class=\"blue-button\" [disabled]=\"wallet.notFound\">{{ 'OPEN_WALLET.MODAL.OPEN' | translate }}</button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"skipWallet()\">{{ 'OPEN_WALLET.MODAL.SKIP' | translate }}</button>\n    </div>\n  </form>\n</div>\n"

/***/ }),

/***/ "./src/app/open-wallet-modal/open-wallet-modal.component.scss":
/*!********************************************************************!*\
  !*** ./src/app/open-wallet-modal/open-wallet-modal.component.scss ***!
  \********************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: fixed;\n  top: 0;\n  bottom: 0;\n  left: 0;\n  right: 0;\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  -webkit-box-pack: center;\n          justify-content: center;\n  background: rgba(255, 255, 255, 0.25); }\n\n.modal {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  background-position: center;\n  background-size: 200%;\n  padding: 2rem;\n  width: 34rem; }\n\n.modal .title {\n    font-size: 1.8rem;\n    text-align: center; }\n\n.modal .open-form .wallet-path {\n    font-size: 1.3rem;\n    margin: 5rem 0 2rem; }\n\n.modal .open-form .wrap-button {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: justify;\n            justify-content: space-between;\n    margin: 2rem -2rem 0; }\n\n.modal .open-form .wrap-button button {\n      -webkit-box-flex: 1;\n              flex: 1 0 0;\n      margin: 0 2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL29wZW4td2FsbGV0LW1vZGFsL29wZW4td2FsbGV0LW1vZGFsLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZUFBZTtFQUNmLE1BQU07RUFDTixTQUFTO0VBQ1QsT0FBTztFQUNQLFFBQVE7RUFDUixvQkFBYTtFQUFiLGFBQWE7RUFDYix5QkFBbUI7VUFBbkIsbUJBQW1CO0VBQ25CLHdCQUF1QjtVQUF2Qix1QkFBdUI7RUFDdkIscUNBQXFDLEVBQUE7O0FBR3ZDO0VBQ0Usb0JBQWE7RUFBYixhQUFhO0VBQ2IsNEJBQXNCO0VBQXRCLDZCQUFzQjtVQUF0QixzQkFBc0I7RUFDdEIsMkJBQTJCO0VBQzNCLHFCQUFxQjtFQUNyQixhQUFhO0VBQ2IsWUFBWSxFQUFBOztBQU5kO0lBU0ksaUJBQWlCO0lBQ2pCLGtCQUFrQixFQUFBOztBQVZ0QjtJQWdCTSxpQkFBaUI7SUFDakIsbUJBQW1CLEVBQUE7O0FBakJ6QjtJQXFCTSxvQkFBYTtJQUFiLGFBQWE7SUFDYix5QkFBbUI7WUFBbkIsbUJBQW1CO0lBQ25CLHlCQUE4QjtZQUE5Qiw4QkFBOEI7SUFDOUIsb0JBQW9CLEVBQUE7O0FBeEIxQjtNQTJCUSxtQkFBVztjQUFYLFdBQVc7TUFDWCxjQUFlLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9vcGVuLXdhbGxldC1tb2RhbC9vcGVuLXdhbGxldC1tb2RhbC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcbiAgcG9zaXRpb246IGZpeGVkO1xuICB0b3A6IDA7XG4gIGJvdHRvbTogMDtcbiAgbGVmdDogMDtcbiAgcmlnaHQ6IDA7XG4gIGRpc3BsYXk6IGZsZXg7XG4gIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG4gIGp1c3RpZnktY29udGVudDogY2VudGVyO1xuICBiYWNrZ3JvdW5kOiByZ2JhKDI1NSwgMjU1LCAyNTUsIDAuMjUpO1xufVxuXG4ubW9kYWwge1xuICBkaXNwbGF5OiBmbGV4O1xuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xuICBiYWNrZ3JvdW5kLXBvc2l0aW9uOiBjZW50ZXI7XG4gIGJhY2tncm91bmQtc2l6ZTogMjAwJTtcbiAgcGFkZGluZzogMnJlbTtcbiAgd2lkdGg6IDM0cmVtO1xuXG4gIC50aXRsZSB7XG4gICAgZm9udC1zaXplOiAxLjhyZW07XG4gICAgdGV4dC1hbGlnbjogY2VudGVyO1xuICB9XG5cbiAgLm9wZW4tZm9ybSB7XG5cbiAgICAud2FsbGV0LXBhdGgge1xuICAgICAgZm9udC1zaXplOiAxLjNyZW07XG4gICAgICBtYXJnaW46IDVyZW0gMCAycmVtO1xuICAgIH1cblxuICAgIC53cmFwLWJ1dHRvbiB7XG4gICAgICBkaXNwbGF5OiBmbGV4O1xuICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcbiAgICAgIG1hcmdpbjogMnJlbSAtMnJlbSAwO1xuXG4gICAgICBidXR0b24ge1xuICAgICAgICBmbGV4OiAxIDAgMDtcbiAgICAgICAgbWFyZ2luOiAwIDJyZW0gO1xuICAgICAgfVxuICAgIH1cbiAgfVxufVxuIl19 */"

/***/ }),

/***/ "./src/app/open-wallet-modal/open-wallet-modal.component.ts":
/*!******************************************************************!*\
  !*** ./src/app/open-wallet-modal/open-wallet-modal.component.ts ***!
  \******************************************************************/
/*! exports provided: OpenWalletModalComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "OpenWalletModalComponent", function() { return OpenWalletModalComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/models/wallet.model */ "./src/app/_helpers/models/wallet.model.ts");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var OpenWalletModalComponent = /** @class */ (function () {
    function OpenWalletModalComponent(variablesService, backend, translate, modalService, ngZone) {
        this.variablesService = variablesService;
        this.backend = backend;
        this.translate = translate;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.wallet = {
            name: '',
            path: '',
            pass: '',
            notFound: false,
            emptyPass: false
        };
    }
    OpenWalletModalComponent.prototype.ngOnInit = function () {
        var _this = this;
        if (this.wallets.length) {
            this.wallet = this.wallets[0];
            this.wallet.pass = '';
            this.backend.openWallet(this.wallet.path, '', true, function (status, data, error) {
                if (error === 'FILE_NOT_FOUND') {
                    _this.wallet.notFound = true;
                }
                if (status) {
                    _this.wallet.pass = '';
                    _this.wallet.emptyPass = true;
                    _this.backend.closeWallet(data.wallet_id);
                    _this.openWallet();
                }
            });
        }
    };
    OpenWalletModalComponent.prototype.openWallet = function () {
        var _this = this;
        if (this.wallets.length === 0) {
            return;
        }
        this.backend.openWallet(this.wallet.path, this.wallet.pass, false, function (open_status, open_data, open_error) {
            if (open_error && open_error === 'FILE_NOT_FOUND') {
                var error_translate = _this.translate.instant('OPEN_WALLET.FILE_NOT_FOUND1');
                error_translate += ':<br>' + _this.wallet.path;
                error_translate += _this.translate.instant('OPEN_WALLET.FILE_NOT_FOUND2');
                _this.modalService.prepareModal('error', error_translate);
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
                        _this.modalService.prepareModal('error', 'OPEN_WALLET.WITH_ADDRESS_ALREADY_OPEN');
                        _this.backend.closeWallet(open_data.wallet_id);
                    }
                    else {
                        var new_wallet_1 = new _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_2__["Wallet"](open_data.wallet_id, _this.wallet.name, _this.wallet.pass, open_data['wi'].path, open_data['wi'].address, open_data['wi'].balance, open_data['wi'].unlocked_balance, open_data['wi'].mined_total, open_data['wi'].tracking_hey);
                        new_wallet_1.alias = _this.backend.getWalletAlias(new_wallet_1.address);
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
                        _this.backend.runWallet(open_data.wallet_id);
                        _this.skipWallet();
                    }
                }
            }
        });
    };
    OpenWalletModalComponent.prototype.skipWallet = function () {
        if (this.wallets.length) {
            this.wallets.splice(0, 1);
            this.ngOnInit();
        }
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", Object)
    ], OpenWalletModalComponent.prototype, "wallets", void 0);
    OpenWalletModalComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-open-wallet-modal',
            template: __webpack_require__(/*! ./open-wallet-modal.component.html */ "./src/app/open-wallet-modal/open-wallet-modal.component.html"),
            styles: [__webpack_require__(/*! ./open-wallet-modal.component.scss */ "./src/app/open-wallet-modal/open-wallet-modal.component.scss")]
        }),
        __metadata("design:paramtypes", [_helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_4__["TranslateService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__["ModalService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], OpenWalletModalComponent);
    return OpenWalletModalComponent;
}());



/***/ }),

/***/ "./src/app/open-wallet/open-wallet.component.html":
/*!********************************************************!*\
  !*** ./src/app/open-wallet/open-wallet.component.html ***!
  \********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\n\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span [routerLink]=\"['/main']\">{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\n      <span>{{ 'BREADCRUMBS.OPEN_WALLET' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" [routerLink]=\"['/main']\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <form class=\"form-open\" [formGroup]=\"openForm\">\n\n    <div class=\"input-block\">\n      <label for=\"wallet-name\">{{ 'OPEN_WALLET.NAME' | translate }}</label>\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [maxLength]=\"variablesService.maxWalletNameLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"openForm.controls['name'].invalid && (openForm.controls['name'].dirty || openForm.controls['name'].touched)\">\n        <div *ngIf=\"openForm.controls['name'].errors['required']\">\n          {{ 'OPEN_WALLET.FORM_ERRORS.NAME_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"openForm.controls['name'].errors['duplicate']\">\n          {{ 'OPEN_WALLET.FORM_ERRORS.NAME_DUPLICATE' | translate }}\n        </div>\n      </div>\n      <div class=\"error-block\" *ngIf=\"openForm.get('name').value.length >= variablesService.maxWalletNameLength\">\n        {{ 'OPEN_WALLET.FORM_ERRORS.MAX_LENGTH' | translate }}\n      </div>\n    </div>\n\n    <div class=\"input-block\">\n      <label for=\"wallet-password\">{{ 'OPEN_WALLET.PASS' | translate }}</label>\n      <input type=\"password\" id=\"wallet-password\" formControlName=\"password\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\n    </div>\n\n    <div class=\"wrap-buttons\">\n      <button type=\"button\" class=\"blue-button create-button\" (click)=\"openWallet()\" [disabled]=\"!openForm.valid\">{{ 'OPEN_WALLET.BUTTON' | translate }}</button>\n    </div>\n\n  </form>\n\n</div>\n\n"

/***/ }),

/***/ "./src/app/open-wallet/open-wallet.component.scss":
/*!********************************************************!*\
  !*** ./src/app/open-wallet/open-wallet.component.scss ***!
  \********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-open {\n  margin: 2.4rem 0;\n  width: 50%; }\n  .form-open .wrap-buttons {\n    display: -webkit-box;\n    display: flex;\n    margin: 2.5rem -0.7rem; }\n  .form-open .wrap-buttons button {\n      margin: 0 0.7rem; }\n  .form-open .wrap-buttons button.create-button {\n        -webkit-box-flex: 1;\n                flex: 1 1 50%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL29wZW4td2FsbGV0L29wZW4td2FsbGV0LmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZ0JBQWdCO0VBQ2hCLFVBQVUsRUFBQTtFQUZaO0lBS0ksb0JBQWE7SUFBYixhQUFhO0lBQ2Isc0JBQXNCLEVBQUE7RUFOMUI7TUFTTSxnQkFBZ0IsRUFBQTtFQVR0QjtRQVlRLG1CQUFhO2dCQUFiLGFBQWEsRUFBQSIsImZpbGUiOiJzcmMvYXBwL29wZW4td2FsbGV0L29wZW4td2FsbGV0LmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tb3BlbiB7XG4gIG1hcmdpbjogMi40cmVtIDA7XG4gIHdpZHRoOiA1MCU7XG5cbiAgLndyYXAtYnV0dG9ucyB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBtYXJnaW46IDIuNXJlbSAtMC43cmVtO1xuXG4gICAgYnV0dG9uIHtcbiAgICAgIG1hcmdpbjogMCAwLjdyZW07XG5cbiAgICAgICYuY3JlYXRlLWJ1dHRvbiB7XG4gICAgICAgIGZsZXg6IDEgMSA1MCU7XG4gICAgICB9XG4gICAgfVxuICB9XG59XG4iXX0= */"

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
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! ../_helpers/models/wallet.model */ "./src/app/_helpers/models/wallet.model.ts");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
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
    function OpenWalletComponent(route, router, backend, variablesService, modalService, ngZone, translate) {
        var _this = this;
        this.route = route;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.translate = translate;
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
        this.queryRouting = this.route.queryParams.subscribe(function (params) {
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
                _this.openForm.get('name').markAsTouched();
            }
        });
    };
    OpenWalletComponent.prototype.openWallet = function () {
        var _this = this;
        if (this.openForm.valid && this.openForm.get('name').value.length <= this.variablesService.maxWalletNameLength) {
            this.backend.openWallet(this.filePath, this.openForm.get('password').value, false, function (open_status, open_data, open_error) {
                if (open_error && open_error === 'FILE_NOT_FOUND') {
                    var error_translate = _this.translate.instant('OPEN_WALLET.FILE_NOT_FOUND1');
                    error_translate += ':<br>' + _this.filePath;
                    error_translate += _this.translate.instant('OPEN_WALLET.FILE_NOT_FOUND2');
                    _this.modalService.prepareModal('error', error_translate);
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
                            _this.modalService.prepareModal('error', 'OPEN_WALLET.WITH_ADDRESS_ALREADY_OPEN');
                            _this.backend.closeWallet(open_data.wallet_id, function () {
                                _this.ngZone.run(function () {
                                    _this.router.navigate(['/']);
                                });
                            });
                        }
                        else {
                            var new_wallet_1 = new _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_6__["Wallet"](open_data.wallet_id, _this.openForm.get('name').value, _this.openForm.get('password').value, open_data['wi'].path, open_data['wi'].address, open_data['wi'].balance, open_data['wi'].unlocked_balance, open_data['wi'].mined_total, open_data['wi'].tracking_hey);
                            new_wallet_1.alias = _this.backend.getWalletAlias(new_wallet_1.address);
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
                            _this.backend.runWallet(open_data.wallet_id, function (run_status, run_data) {
                                if (run_status) {
                                    if (_this.variablesService.appPass) {
                                        _this.backend.storeSecureAppData();
                                    }
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
    OpenWalletComponent.prototype.ngOnDestroy = function () {
        this.queryRouting.unsubscribe();
    };
    OpenWalletComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-open-wallet',
            template: __webpack_require__(/*! ./open-wallet.component.html */ "./src/app/open-wallet/open-wallet.component.html"),
            styles: [__webpack_require__(/*! ./open-wallet.component.scss */ "./src/app/open-wallet/open-wallet.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_5__["ActivatedRoute"],
            _angular_router__WEBPACK_IMPORTED_MODULE_5__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__["ModalService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_7__["TranslateService"]])
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

module.exports = "<div class=\"head\">\n  <div class=\"breadcrumbs\">\n    <span [routerLink]=\"'/wallet/' + currentWalletId + '/contracts'\">{{ 'BREADCRUMBS.CONTRACTS' | translate }}</span>\n    <span *ngIf=\"newPurchase\">{{ 'BREADCRUMBS.NEW_PURCHASE' | translate }}</span>\n    <span *ngIf=\"!newPurchase\">{{ 'BREADCRUMBS.OLD_PURCHASE' | translate }}</span>\n  </div>\n  <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n    <i class=\"icon back\"></i>\n    <span>{{ 'COMMON.BACK' | translate }}</span>\n  </button>\n</div>\n\n<form class=\"form-purchase scrolled-content\" [formGroup]=\"purchaseForm\">\n\n  <div class=\"input-block\">\n    <label for=\"purchase-description\">{{ 'PURCHASE.DESCRIPTION' | translate }}</label>\n    <input type=\"text\" id=\"purchase-description\" formControlName=\"description\" maxlength=\"100\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n    <div class=\"error-block\" *ngIf=\"purchaseForm.controls['description'].invalid && (purchaseForm.controls['description'].dirty || purchaseForm.controls['description'].touched)\">\n      <div *ngIf=\"purchaseForm.controls['description'].errors['required']\">\n        {{ 'PURCHASE.FORM_ERRORS.DESC_REQUIRED' | translate }}\n      </div>\n    </div>\n    <div class=\"error-block\" *ngIf=\"newPurchase && purchaseForm.controls['description'].value.length >= 100\">\n      <div>\n        {{ 'PURCHASE.FORM_ERRORS.COMMENT_MAXIMUM' | translate }}\n      </div>\n    </div>\n  </div>\n\n  <div class=\"input-blocks-row\">\n    <div class=\"input-block input-block-alias\">\n      <label for=\"purchase-seller\">{{ 'PURCHASE.SELLER' | translate }}</label>\n      <input type=\"text\" id=\"purchase-seller\" formControlName=\"seller\" [readonly]=\"!newPurchase\" (mousedown)=\"addressMouseDown($event)\" (contextmenu)=\"(!newPurchase) ? variablesService.onContextMenuOnlyCopy($event, purchaseForm.controls['seller'].value) : variablesService.onContextMenu($event)\">\n      <div class=\"alias-dropdown scrolled-content\" *ngIf=\"isOpen\">\n        <div *ngFor=\"let item of localAliases\" (click)=\"setAlias(item.name)\">{{item.name}}</div>\n      </div>\n      <div class=\"error-block\" *ngIf=\"purchaseForm.controls['seller'].invalid && (purchaseForm.controls['seller'].dirty || purchaseForm.controls['seller'].touched)\">\n        <div *ngIf=\"purchaseForm.controls['seller'].errors['required']\">\n          {{ 'PURCHASE.FORM_ERRORS.SELLER_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"purchaseForm.controls['seller'].errors['address_not_valid']\">\n          {{ 'PURCHASE.FORM_ERRORS.SELLER_NOT_VALID' | translate }}\n        </div>\n        <div *ngIf=\"purchaseForm.controls['seller'].errors['address_same']\">\n          {{ 'PURCHASE.FORM_ERRORS.SELLER_SAME' | translate }}\n        </div>\n        <div *ngIf=\"purchaseForm.controls['seller'].errors['alias_not_valid']\">\n          {{ 'PURCHASE.FORM_ERRORS.ALIAS_NOT_VALID' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"input-block\">\n      <label for=\"purchase-amount\">{{ 'PURCHASE.AMOUNT' | translate }}</label>\n      <input type=\"text\" id=\"purchase-amount\" formControlName=\"amount\" appInputValidate=\"money\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"purchaseForm.controls['amount'].invalid && (purchaseForm.controls['amount'].dirty || purchaseForm.controls['amount'].touched)\">\n        <div *ngIf=\"purchaseForm.controls['amount'].errors['required']\">\n          {{ 'PURCHASE.FORM_ERRORS.AMOUNT_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"purchaseForm.controls['amount'].errors['amount_zero']\">\n          {{ 'PURCHASE.FORM_ERRORS.AMOUNT_ZERO' | translate }}\n        </div>\n      </div>\n    </div>\n  </div>\n\n  <div class=\"input-blocks-row\">\n    <div class=\"input-block\">\n      <label for=\"purchase-your-deposit\">{{ ( (currentContract && !currentContract.is_a) ? 'PURCHASE.BUYER_DEPOSIT' : 'PURCHASE.YOUR_DEPOSIT') | translate }}</label>\n      <input type=\"text\" id=\"purchase-your-deposit\" formControlName=\"yourDeposit\" appInputValidate=\"money\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"purchaseForm.controls['yourDeposit'].invalid && (purchaseForm.controls['yourDeposit'].dirty || purchaseForm.controls['yourDeposit'].touched)\">\n        <div *ngIf=\"purchaseForm.controls['yourDeposit'].errors['required']\">\n          {{ 'PURCHASE.FORM_ERRORS.YOUR_DEPOSIT_REQUIRED' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"input-block\">\n      <div class=\"wrap-label\">\n        <label for=\"purchase-seller-deposit\">{{ ( (currentContract && !currentContract.is_a) ? 'PURCHASE.YOUR_DEPOSIT' : 'PURCHASE.SELLER_DEPOSIT') | translate }}</label>\n        <div class=\"checkbox-block\">\n          <input type=\"checkbox\" id=\"purchase-same-amount\" class=\"style-checkbox\" formControlName=\"sameAmount\" (change)=\"sameAmountChange()\">\n          <label for=\"purchase-same-amount\">{{ 'PURCHASE.SAME_AMOUNT' | translate }}</label>\n        </div>\n      </div>\n      <input type=\"text\" readonly *ngIf=\"purchaseForm.controls['sameAmount'].value\" [value]=\"purchaseForm.controls['amount'].value\">\n      <input type=\"text\" id=\"purchase-seller-deposit\" *ngIf=\"!purchaseForm.controls['sameAmount'].value\" formControlName=\"sellerDeposit\" appInputValidate=\"money\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"purchaseForm.controls['sellerDeposit'].invalid && (purchaseForm.controls['sellerDeposit'].dirty || purchaseForm.controls['sellerDeposit'].touched)\">\n        <div *ngIf=\"purchaseForm.controls['sellerDeposit'].errors['required']\">\n          {{ 'PURCHASE.FORM_ERRORS.SELLER_DEPOSIT_REQUIRED' | translate }}\n        </div>\n      </div>\n    </div>\n  </div>\n\n  <div class=\"input-block\">\n    <label for=\"purchase-comment\">{{ 'PURCHASE.COMMENT' | translate }}</label>\n    <input type=\"text\" id=\"purchase-comment\" formControlName=\"comment\" maxlength=\"100\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n    <div class=\"error-block\" *ngIf=\"newPurchase && purchaseForm.controls['comment'].value.length >= 100\">\n      <div>\n        {{ 'PURCHASE.FORM_ERRORS.COMMENT_MAXIMUM' | translate }}\n      </div>\n    </div>\n  </div>\n\n  <button type=\"button\" class=\"purchase-select\" (click)=\"toggleOptions()\">\n    <span>{{ 'PURCHASE.DETAILS' | translate }}</span><i class=\"icon arrow\" [class.down]=\"!additionalOptions\" [class.up]=\"additionalOptions\"></i>\n  </button>\n\n  <div class=\"additional-details\" *ngIf=\"additionalOptions\">\n    <div class=\"input-block\">\n      <label for=\"purchase-fee\">{{ 'PURCHASE.FEE' | translate }}</label>\n      <input type=\"text\" id=\"purchase-fee\" formControlName=\"fee\" readonly>\n    </div>\n    <div class=\"input-block\" *ngIf=\"newPurchase\">\n      <label for=\"purchase-time\">{{ 'PURCHASE.WAITING_TIME' | translate }}</label>\n      <ng-select id=\"purchase-time\" class=\"custom-select\"\n                 [clearable]=\"false\"\n                 [searchable]=\"false\"\n                 formControlName=\"time\">\n        <ng-option [value]=\"1\">1 {{ 'PURCHASE.HOUR' | translate }}</ng-option>\n        <ng-option *ngFor=\"let title of [2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]\" [value]=\"title\">\n          {{title}} {{ 'PURCHASE.HOURS' | translate }}\n        </ng-option>\n      </ng-select>\n    </div>\n    <div class=\"input-block\">\n      <label for=\"purchase-payment\">{{ 'PURCHASE.PAYMENT' | translate }}</label>\n      <input type=\"text\" id=\"purchase-payment\" formControlName=\"payment\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n    </div>\n  </div>\n\n  <button type=\"button\" class=\"blue-button send-button\" *ngIf=\"newPurchase\" [disabled]=\"!purchaseForm.valid\" (click)=\"createPurchase()\">{{ 'PURCHASE.SEND_BUTTON' | translate }}</button>\n\n  <div class=\"purchase-states\" *ngIf=\"!newPurchase\">\n    <ng-container *ngIf=\"currentContract.state == 1 && !currentContract.is_a && currentContract.private_detailes.b_pledge.plus(variablesService.default_fee_big).plus(variablesService.default_fee_big).isGreaterThan(variablesService.currentWallet.unlocked_balance)\">\n      <span>{{ 'PURCHASE.NEED_MONEY' | translate }}</span>\n    </ng-container>\n  </div>\n\n  <div class=\"purchase-buttons\" *ngIf=\"!newPurchase\">\n\n    <ng-container *ngIf=\"!currentContract.is_a && currentContract.state == 1\">\n      <button type=\"button\" class=\"green-button\" (click)=\"acceptState();\" [disabled]=\"currentContract.private_detailes.b_pledge.plus(variablesService.default_fee_big).plus(variablesService.default_fee_big).isGreaterThan(variablesService.currentWallet.unlocked_balance)\">\n        {{'PURCHASE.BUTTON_MAKE_PLEDGE' | translate}}\n      </button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"ignoredContract();\">{{'PURCHASE.BUTTON_IGNORE' | translate}}</button>\n    </ng-container>\n\n    <ng-container *ngIf=\"!showNullify && !showTimeSelect && currentContract.is_a && (currentContract.state == 201 || currentContract.state == 2 || currentContract.state == 120 || currentContract.state == 130)\">\n      <button type=\"button\" class=\"green-button\" (click)=\"dealsDetailsFinish();\" [disabled]=\"currentContract.cancel_expiration_time == 0 && (currentContract.height == 0 || (variablesService.height_app - currentContract.height) < 10)\">\n        {{'PURCHASE.BUTTON_RECEIVED' | translate}}\n      </button>\n      <button type=\"button\" class=\"turquoise-button\" (click)=\"showNullify = true;\" [disabled]=\"currentContract.cancel_expiration_time == 0 && (currentContract.height == 0 || (variablesService.height_app - currentContract.height) < 10)\">\n        {{'PURCHASE.BUTTON_NULLIFY' | translate}}\n      </button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"showTimeSelect = true;\" [disabled]=\"currentContract.cancel_expiration_time == 0 && (currentContract.height == 0 || (variablesService.height_app - currentContract.height) < 10)\">\n        {{'PURCHASE.BUTTON_CANCEL_BUYER' | translate}}\n      </button>\n    </ng-container>\n\n    <ng-container *ngIf=\"!currentContract.is_a && currentContract.state == 5\">\n      <button type=\"button\" class=\"turquoise-button\" (click)=\"dealsDetailsDontCanceling();\">{{'PURCHASE.BUTTON_NOT_CANCEL' | translate}}</button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"dealsDetailsSellerCancel();\">{{'PURCHASE.BUTTON_CANCEL_SELLER' | translate}}</button>\n    </ng-container>\n\n  </div>\n\n  <div class=\"nullify-block-row\" *ngIf=\"showNullify\">\n    <div>{{'PURCHASE.NULLIFY_QUESTION' | translate}}</div>\n    <div class=\"nullify-block-buttons\">\n      <button type=\"button\" class=\"blue-button\" (click)=\"showNullify = false;\">{{ 'PURCHASE.CANCEL' | translate }}</button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"productNotGot();\">{{ 'PURCHASE.BUTTON_NULLIFY_SHORT' | translate }}</button>\n    </div>\n  </div>\n\n  <div class=\"time-cancel-block-row\" *ngIf=\"showTimeSelect && !newPurchase && currentContract.is_a && (currentContract.state == 201 || currentContract.state == 2 || currentContract.state == 120 || currentContract.state == 130)\">\n    <div class=\"time-cancel-block-question\">{{ 'PURCHASE.WAITING_TIME_QUESTION' | translate }}</div>\n    <label for=\"purchase-timeCancel\">{{ 'PURCHASE.WAITING_TIME' | translate }}</label>\n    <div class=\"input-block\">\n      <ng-select id=\"purchase-timeCancel\" class=\"custom-select\"\n                 [clearable]=\"false\"\n                 [searchable]=\"false\"\n                 formControlName=\"timeCancel\">\n        <ng-option [value]=\"1\">1 {{ 'PURCHASE.HOUR' | translate }}</ng-option>\n        <ng-option *ngFor=\"let title of [2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]\" [value]=\"title\">\n          {{title}} {{ 'PURCHASE.HOURS' | translate }}\n        </ng-option>\n      </ng-select>\n    </div>\n    <div class=\"time-cancel-block-buttons\">\n      <button type=\"button\" class=\"blue-button\" (click)=\"showTimeSelect = false;\">{{ 'PURCHASE.CANCEL' | translate }}</button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"dealsDetailsCancel();\">{{ 'PURCHASE.BUTTON_CANCEL_BUYER' | translate }}</button>\n    </div>\n  </div>\n\n</form>\n\n<div class=\"progress-bar-container\">\n  <div class=\"progress-bar\">\n    <div class=\"progress-bar-full\" [style.width]=\"getProgressBarWidth()\"></div>\n  </div>\n  <div class=\"progress-labels\">\n\n    <ng-container *ngIf=\"newPurchase\">\n      <span>{{ 'PURCHASE.STATUS_MESSAGES.NEW_PURCHASE' | translate }}</span>\n    </ng-container>\n\n    <ng-container *ngIf=\"!newPurchase && currentContract.is_a\">\n      <span *ngIf=\"currentContract.state == 1\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_SELLER' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 110\">{{ 'PURCHASE.STATUS_MESSAGES.IGNORED_SELLER' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 120\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_DELIVERY' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 130\">{{ 'PURCHASE.STATUS_MESSAGES.IGNORED_CANCEL_SELLER' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 140\">{{ 'PURCHASE.STATUS_MESSAGES.EXPIRED' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 2\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_SELLER' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 201\">\n        {{ 'PURCHASE.STATUS_MESSAGES.WAITING_CONFIRMATION' | translate }}\n        <ng-container *ngIf=\"currentContract.height === 0\">(0/10)</ng-container>\n        <ng-container *ngIf=\"currentContract.height !== 0 && (variablesService.height_app - currentContract.height) < 10\">({{variablesService.height_app - currentContract.height}}/10)</ng-container>\n      </span>\n\n      <span *ngIf=\"currentContract.state == 3\">{{ 'PURCHASE.STATUS_MESSAGES.COMPLETED' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 4\" class=\"error-text\">\n        {{ 'PURCHASE.STATUS_MESSAGES.NOT_RECEIVED' | translate }}. {{ 'PURCHASE.STATUS_MESSAGES.NULLIFIED' | translate }}\n      </span>\n\n      <span *ngIf=\"currentContract.state == 5\">{{ 'PURCHASE.STATUS_MESSAGES.PROPOSAL_CANCEL_SELLER' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 6\">{{ 'PURCHASE.STATUS_MESSAGES.CANCELLED' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 601\">\n        {{ 'PURCHASE.STATUS_MESSAGES.BEING_CANCELLED' | translate }}\n        <ng-container *ngIf=\"currentContract.height === 0\">(0/10)</ng-container>\n        <ng-container *ngIf=\"currentContract.height !== 0 && (variablesService.height_app - currentContract.height) < 10\">({{variablesService.height_app - currentContract.height}}/10)</ng-container>\n      </span>\n    </ng-container>\n\n    <ng-container *ngIf=\"!newPurchase && !currentContract.is_a\">\n      <span *ngIf=\"currentContract.state == 1\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_BUYER' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 110\">{{ 'PURCHASE.STATUS_MESSAGES.IGNORED_BUYER' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 130\">{{ 'PURCHASE.STATUS_MESSAGES.IGNORED_CANCEL_BUYER' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 140\">{{ 'PURCHASE.STATUS_MESSAGES.EXPIRED' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 2\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_DELIVERY' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 201\">\n        {{ 'PURCHASE.STATUS_MESSAGES.WAITING_CONFIRMATION' | translate }}\n        <ng-container *ngIf=\"currentContract.height === 0\">(0/10)</ng-container>\n        <ng-container *ngIf=\"currentContract.height !== 0 && (variablesService.height_app - currentContract.height) < 10\">({{variablesService.height_app - currentContract.height}}/10)</ng-container>\n      </span>\n\n      <span *ngIf=\"currentContract.state == 3\">{{ 'PURCHASE.STATUS_MESSAGES.COMPLETED' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 4\" class=\"error-text\">\n        {{ 'PURCHASE.STATUS_MESSAGES.NOT_RECEIVED' | translate }}. {{ 'PURCHASE.STATUS_MESSAGES.NULLIFIED' | translate }}\n      </span>\n\n      <span *ngIf=\"currentContract.state == 5\">{{ 'PURCHASE.STATUS_MESSAGES.PROPOSAL_CANCEL_BUYER' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 6\">{{ 'PURCHASE.STATUS_MESSAGES.CANCELLED' | translate }}</span>\n\n      <span *ngIf=\"currentContract.state == 601\">\n        {{ 'PURCHASE.STATUS_MESSAGES.BEING_CANCELLED' | translate }}\n        <ng-container *ngIf=\"currentContract.height === 0\">(0/10)</ng-container>\n        <ng-container *ngIf=\"currentContract.height !== 0 && (variablesService.height_app - currentContract.height) < 10\">({{variablesService.height_app - currentContract.height}}/10)</ng-container>\n      </span>\n    </ng-container>\n\n  </div>\n  <div class=\"progress-time\" *ngIf=\"!newPurchase\">\n    <span *ngIf=\"currentContract.is_a && currentContract.state == 1\">{{currentContract.expiration_time | contractTimeLeft: 0}}</span>\n    <span *ngIf=\"currentContract.is_a && currentContract.state == 5\">{{currentContract.cancel_expiration_time | contractTimeLeft: 2}}</span>\n    <span *ngIf=\"!currentContract.is_a && currentContract.state == 1\">{{currentContract.expiration_time | contractTimeLeft: 1}}</span>\n    <span *ngIf=\"!currentContract.is_a && currentContract.state == 5\">{{currentContract.cancel_expiration_time | contractTimeLeft: 1}}</span>\n  </div>\n</div>\n"

/***/ }),

/***/ "./src/app/purchase/purchase.component.scss":
/*!**************************************************!*\
  !*** ./src/app/purchase/purchase.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  width: 100%; }\n\n.head {\n  -webkit-box-flex: 0;\n          flex: 0 0 auto;\n  box-sizing: content-box;\n  margin: -3rem -3rem 0; }\n\n.form-purchase {\n  -webkit-box-flex: 1;\n          flex: 1 1 auto;\n  margin: 1.5rem -3rem 0;\n  padding: 0 3rem;\n  overflow-y: overlay; }\n\n.form-purchase .input-blocks-row {\n    display: -webkit-box;\n    display: flex; }\n\n.form-purchase .input-blocks-row .input-block {\n      flex-basis: 50%; }\n\n.form-purchase .input-blocks-row .input-block:first-child {\n        margin-right: 1.5rem; }\n\n.form-purchase .input-blocks-row .input-block:last-child {\n        margin-left: 1.5rem; }\n\n.form-purchase .input-blocks-row .input-block .checkbox-block {\n        display: -webkit-box;\n        display: flex; }\n\n.form-purchase .purchase-select {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    background: transparent;\n    border: none;\n    font-size: 1.3rem;\n    line-height: 1.3rem;\n    margin: 1.5rem 0 0;\n    padding: 0;\n    width: 100%;\n    max-width: 15rem;\n    height: 1.3rem; }\n\n.form-purchase .purchase-select .arrow {\n      margin-left: 1rem;\n      width: 0.8rem;\n      height: 0.8rem; }\n\n.form-purchase .purchase-select .arrow.down {\n        -webkit-mask: url('arrow-down.svg') no-repeat center;\n                mask: url('arrow-down.svg') no-repeat center; }\n\n.form-purchase .purchase-select .arrow.up {\n        -webkit-mask: url('arrow-up.svg') no-repeat center;\n                mask: url('arrow-up.svg') no-repeat center; }\n\n.form-purchase .additional-details {\n    display: -webkit-box;\n    display: flex;\n    margin-top: 1.5rem;\n    padding: 0.5rem 0 2rem; }\n\n.form-purchase .additional-details > div {\n      flex-basis: 25%; }\n\n.form-purchase .additional-details > div:first-child {\n        padding-left: 1.5rem;\n        padding-right: 1rem; }\n\n.form-purchase .additional-details > div:last-child {\n        padding-left: 1rem;\n        padding-right: 1.5rem; }\n\n.form-purchase .purchase-states {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-orient: vertical;\n    -webkit-box-direction: normal;\n            flex-direction: column;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: center;\n            justify-content: center;\n    font-size: 1.2rem;\n    line-height: 2.9rem; }\n\n.form-purchase .send-button {\n    margin: 2.4rem 0;\n    width: 100%;\n    max-width: 15rem; }\n\n.form-purchase .purchase-buttons {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-pack: start;\n            justify-content: flex-start;\n    margin: 2.4rem -0.5rem; }\n\n.form-purchase .purchase-buttons button {\n      -webkit-box-flex: 0;\n              flex: 0 1 33%;\n      margin: 0 0.5rem; }\n\n.form-purchase .nullify-block-row {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-orient: vertical;\n    -webkit-box-direction: normal;\n            flex-direction: column;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: center;\n            justify-content: center; }\n\n.form-purchase .nullify-block-row .nullify-block-buttons {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-align: center;\n              align-items: center;\n      -webkit-box-pack: center;\n              justify-content: center;\n      margin: 1rem 0;\n      width: 100%; }\n\n.form-purchase .nullify-block-row .nullify-block-buttons button {\n        -webkit-box-flex: 0;\n                flex: 0 1 25%;\n        margin: 0 0.5rem; }\n\n.form-purchase .time-cancel-block-row {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-orient: vertical;\n    -webkit-box-direction: normal;\n            flex-direction: column;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: center;\n            justify-content: center; }\n\n.form-purchase .time-cancel-block-row .time-cancel-block-question {\n      margin-bottom: 1rem; }\n\n.form-purchase .time-cancel-block-row .input-block {\n      width: 25%; }\n\n.form-purchase .time-cancel-block-row label {\n      margin-bottom: 1rem; }\n\n.form-purchase .time-cancel-block-row .time-cancel-block-buttons {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-align: center;\n              align-items: center;\n      -webkit-box-pack: center;\n              justify-content: center;\n      margin: 1rem 0;\n      width: 100%; }\n\n.form-purchase .time-cancel-block-row .time-cancel-block-buttons button {\n        -webkit-box-flex: 0;\n                flex: 0 1 25%;\n        margin: 0 0.5rem; }\n\n.progress-bar-container {\n  position: absolute;\n  bottom: 0;\n  left: 0;\n  padding: 0 3rem;\n  width: 100%;\n  height: 3rem; }\n\n.progress-bar-container .progress-bar {\n    position: absolute;\n    top: -0.7rem;\n    left: 0;\n    margin: 0 3rem;\n    width: calc(100% - 6rem);\n    height: 0.7rem; }\n\n.progress-bar-container .progress-bar .progress-bar-full {\n      height: 0.7rem; }\n\n.progress-bar-container .progress-labels {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: center;\n            justify-content: center;\n    font-size: 1.2rem;\n    height: 100%; }\n\n.progress-bar-container .progress-time {\n    position: absolute;\n    top: -3rem;\n    left: 50%;\n    -webkit-transform: translateX(-50%);\n            transform: translateX(-50%);\n    font-size: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3B1cmNoYXNlL3B1cmNoYXNlLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usb0JBQWE7RUFBYixhQUFhO0VBQ2IsNEJBQXNCO0VBQXRCLDZCQUFzQjtVQUF0QixzQkFBc0I7RUFDdEIsV0FBVyxFQUFBOztBQUdiO0VBQ0UsbUJBQWM7VUFBZCxjQUFjO0VBQ2QsdUJBQXVCO0VBQ3ZCLHFCQUFxQixFQUFBOztBQUd2QjtFQUNFLG1CQUFjO1VBQWQsY0FBYztFQUNkLHNCQUFzQjtFQUN0QixlQUFlO0VBQ2YsbUJBQW1CLEVBQUE7O0FBSnJCO0lBT0ksb0JBQWE7SUFBYixhQUFhLEVBQUE7O0FBUGpCO01BVU0sZUFBZSxFQUFBOztBQVZyQjtRQWFRLG9CQUFvQixFQUFBOztBQWI1QjtRQWlCUSxtQkFBbUIsRUFBQTs7QUFqQjNCO1FBcUJRLG9CQUFhO1FBQWIsYUFBYSxFQUFBOztBQXJCckI7SUEyQkksb0JBQWE7SUFBYixhQUFhO0lBQ2IseUJBQW1CO1lBQW5CLG1CQUFtQjtJQUNuQix1QkFBdUI7SUFDdkIsWUFBWTtJQUNaLGlCQUFpQjtJQUNqQixtQkFBbUI7SUFDbkIsa0JBQWtCO0lBQ2xCLFVBQVU7SUFDVixXQUFXO0lBQ1gsZ0JBQWdCO0lBQ2hCLGNBQWMsRUFBQTs7QUFyQ2xCO01Bd0NNLGlCQUFpQjtNQUNqQixhQUFhO01BQ2IsY0FBYyxFQUFBOztBQTFDcEI7UUE2Q1Esb0RBQTREO2dCQUE1RCw0Q0FBNEQsRUFBQTs7QUE3Q3BFO1FBaURRLGtEQUEwRDtnQkFBMUQsMENBQTBELEVBQUE7O0FBakRsRTtJQXVESSxvQkFBYTtJQUFiLGFBQWE7SUFDYixrQkFBa0I7SUFDbEIsc0JBQXNCLEVBQUE7O0FBekQxQjtNQTRETSxlQUFlLEVBQUE7O0FBNURyQjtRQStEUSxvQkFBb0I7UUFDcEIsbUJBQW1CLEVBQUE7O0FBaEUzQjtRQW9FUSxrQkFBa0I7UUFDbEIscUJBQXFCLEVBQUE7O0FBckU3QjtJQTJFSSxvQkFBYTtJQUFiLGFBQWE7SUFDYiw0QkFBc0I7SUFBdEIsNkJBQXNCO1lBQXRCLHNCQUFzQjtJQUN0Qix5QkFBbUI7WUFBbkIsbUJBQW1CO0lBQ25CLHdCQUF1QjtZQUF2Qix1QkFBdUI7SUFDdkIsaUJBQWlCO0lBQ2pCLG1CQUFtQixFQUFBOztBQWhGdkI7SUFvRkksZ0JBQWdCO0lBQ2hCLFdBQVc7SUFDWCxnQkFBZ0IsRUFBQTs7QUF0RnBCO0lBMEZJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLHVCQUEyQjtZQUEzQiwyQkFBMkI7SUFDM0Isc0JBQXNCLEVBQUE7O0FBNUYxQjtNQStGTSxtQkFBYTtjQUFiLGFBQWE7TUFDYixnQkFBZ0IsRUFBQTs7QUFoR3RCO0lBcUdJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLDRCQUFzQjtJQUF0Qiw2QkFBc0I7WUFBdEIsc0JBQXNCO0lBQ3RCLHlCQUFtQjtZQUFuQixtQkFBbUI7SUFDbkIsd0JBQXVCO1lBQXZCLHVCQUF1QixFQUFBOztBQXhHM0I7TUEyR00sb0JBQWE7TUFBYixhQUFhO01BQ2IseUJBQW1CO2NBQW5CLG1CQUFtQjtNQUNuQix3QkFBdUI7Y0FBdkIsdUJBQXVCO01BQ3ZCLGNBQWM7TUFDZCxXQUFXLEVBQUE7O0FBL0dqQjtRQWtIUSxtQkFBYTtnQkFBYixhQUFhO1FBQ2IsZ0JBQWdCLEVBQUE7O0FBbkh4QjtJQXlISSxvQkFBYTtJQUFiLGFBQWE7SUFDYiw0QkFBc0I7SUFBdEIsNkJBQXNCO1lBQXRCLHNCQUFzQjtJQUN0Qix5QkFBbUI7WUFBbkIsbUJBQW1CO0lBQ25CLHdCQUF1QjtZQUF2Qix1QkFBdUIsRUFBQTs7QUE1SDNCO01BK0hNLG1CQUFtQixFQUFBOztBQS9IekI7TUFtSU0sVUFBVSxFQUFBOztBQW5JaEI7TUF1SU0sbUJBQW1CLEVBQUE7O0FBdkl6QjtNQTJJTSxvQkFBYTtNQUFiLGFBQWE7TUFDYix5QkFBbUI7Y0FBbkIsbUJBQW1CO01BQ25CLHdCQUF1QjtjQUF2Qix1QkFBdUI7TUFDdkIsY0FBYztNQUNkLFdBQVcsRUFBQTs7QUEvSWpCO1FBa0pRLG1CQUFhO2dCQUFiLGFBQWE7UUFDYixnQkFBZ0IsRUFBQTs7QUFPeEI7RUFDRSxrQkFBa0I7RUFDbEIsU0FBUztFQUNULE9BQU87RUFDUCxlQUFlO0VBQ2YsV0FBVztFQUNYLFlBQVksRUFBQTs7QUFOZDtJQVNJLGtCQUFrQjtJQUNsQixZQUFZO0lBQ1osT0FBTztJQUNQLGNBQWM7SUFDZCx3QkFBd0I7SUFDeEIsY0FBYyxFQUFBOztBQWRsQjtNQWlCTSxjQUFjLEVBQUE7O0FBakJwQjtJQXNCSSxvQkFBYTtJQUFiLGFBQWE7SUFDYix5QkFBbUI7WUFBbkIsbUJBQW1CO0lBQ25CLHdCQUF1QjtZQUF2Qix1QkFBdUI7SUFDdkIsaUJBQWlCO0lBQ2pCLFlBQVksRUFBQTs7QUExQmhCO0lBOEJJLGtCQUFrQjtJQUNsQixVQUFVO0lBQ1YsU0FBUztJQUNULG1DQUEyQjtZQUEzQiwyQkFBMkI7SUFDM0IsaUJBQWlCLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9wdXJjaGFzZS9wdXJjaGFzZS5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcbiAgZGlzcGxheTogZmxleDtcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcbiAgd2lkdGg6IDEwMCU7XG59XG5cbi5oZWFkIHtcbiAgZmxleDogMCAwIGF1dG87XG4gIGJveC1zaXppbmc6IGNvbnRlbnQtYm94O1xuICBtYXJnaW46IC0zcmVtIC0zcmVtIDA7XG59XG5cbi5mb3JtLXB1cmNoYXNlIHtcbiAgZmxleDogMSAxIGF1dG87XG4gIG1hcmdpbjogMS41cmVtIC0zcmVtIDA7XG4gIHBhZGRpbmc6IDAgM3JlbTtcbiAgb3ZlcmZsb3cteTogb3ZlcmxheTtcblxuICAuaW5wdXQtYmxvY2tzLXJvdyB7XG4gICAgZGlzcGxheTogZmxleDtcblxuICAgIC5pbnB1dC1ibG9jayB7XG4gICAgICBmbGV4LWJhc2lzOiA1MCU7XG5cbiAgICAgICY6Zmlyc3QtY2hpbGQge1xuICAgICAgICBtYXJnaW4tcmlnaHQ6IDEuNXJlbTtcbiAgICAgIH1cblxuICAgICAgJjpsYXN0LWNoaWxkIHtcbiAgICAgICAgbWFyZ2luLWxlZnQ6IDEuNXJlbTtcbiAgICAgIH1cblxuICAgICAgLmNoZWNrYm94LWJsb2NrIHtcbiAgICAgICAgZGlzcGxheTogZmxleDtcbiAgICAgIH1cbiAgICB9XG4gIH1cblxuICAucHVyY2hhc2Utc2VsZWN0IHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG4gICAgYmFja2dyb3VuZDogdHJhbnNwYXJlbnQ7XG4gICAgYm9yZGVyOiBub25lO1xuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xuICAgIGxpbmUtaGVpZ2h0OiAxLjNyZW07XG4gICAgbWFyZ2luOiAxLjVyZW0gMCAwO1xuICAgIHBhZGRpbmc6IDA7XG4gICAgd2lkdGg6IDEwMCU7XG4gICAgbWF4LXdpZHRoOiAxNXJlbTtcbiAgICBoZWlnaHQ6IDEuM3JlbTtcblxuICAgIC5hcnJvdyB7XG4gICAgICBtYXJnaW4tbGVmdDogMXJlbTtcbiAgICAgIHdpZHRoOiAwLjhyZW07XG4gICAgICBoZWlnaHQ6IDAuOHJlbTtcblxuICAgICAgJi5kb3duIHtcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL2Fycm93LWRvd24uc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xuICAgICAgfVxuXG4gICAgICAmLnVwIHtcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL2Fycm93LXVwLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgIH1cbiAgICB9XG4gIH1cblxuICAuYWRkaXRpb25hbC1kZXRhaWxzIHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIG1hcmdpbi10b3A6IDEuNXJlbTtcbiAgICBwYWRkaW5nOiAwLjVyZW0gMCAycmVtO1xuXG4gICAgPiBkaXYge1xuICAgICAgZmxleC1iYXNpczogMjUlO1xuXG4gICAgICAmOmZpcnN0LWNoaWxkIHtcbiAgICAgICAgcGFkZGluZy1sZWZ0OiAxLjVyZW07XG4gICAgICAgIHBhZGRpbmctcmlnaHQ6IDFyZW07XG4gICAgICB9XG5cbiAgICAgICY6bGFzdC1jaGlsZCB7XG4gICAgICAgIHBhZGRpbmctbGVmdDogMXJlbTtcbiAgICAgICAgcGFkZGluZy1yaWdodDogMS41cmVtO1xuICAgICAgfVxuICAgIH1cbiAgfVxuXG4gIC5wdXJjaGFzZS1zdGF0ZXMge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xuICAgIGZvbnQtc2l6ZTogMS4ycmVtO1xuICAgIGxpbmUtaGVpZ2h0OiAyLjlyZW07XG4gIH1cblxuICAuc2VuZC1idXR0b24ge1xuICAgIG1hcmdpbjogMi40cmVtIDA7XG4gICAgd2lkdGg6IDEwMCU7XG4gICAgbWF4LXdpZHRoOiAxNXJlbTtcbiAgfVxuXG4gIC5wdXJjaGFzZS1idXR0b25zIHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIGp1c3RpZnktY29udGVudDogZmxleC1zdGFydDtcbiAgICBtYXJnaW46IDIuNHJlbSAtMC41cmVtO1xuXG4gICAgYnV0dG9uIHtcbiAgICAgIGZsZXg6IDAgMSAzMyU7XG4gICAgICBtYXJnaW46IDAgMC41cmVtO1xuICAgIH1cbiAgfVxuXG4gIC5udWxsaWZ5LWJsb2NrLXJvdyB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG4gICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XG5cbiAgICAubnVsbGlmeS1ibG9jay1idXR0b25zIHtcbiAgICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XG4gICAgICBtYXJnaW46IDFyZW0gMDtcbiAgICAgIHdpZHRoOiAxMDAlO1xuXG4gICAgICBidXR0b24ge1xuICAgICAgICBmbGV4OiAwIDEgMjUlO1xuICAgICAgICBtYXJnaW46IDAgMC41cmVtO1xuICAgICAgfVxuICAgIH1cbiAgfVxuXG4gIC50aW1lLWNhbmNlbC1ibG9jay1yb3cge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xuXG4gICAgLnRpbWUtY2FuY2VsLWJsb2NrLXF1ZXN0aW9uIHtcbiAgICAgIG1hcmdpbi1ib3R0b206IDFyZW07XG4gICAgfVxuXG4gICAgLmlucHV0LWJsb2NrIHtcbiAgICAgIHdpZHRoOiAyNSU7XG4gICAgfVxuXG4gICAgbGFiZWwge1xuICAgICAgbWFyZ2luLWJvdHRvbTogMXJlbTtcbiAgICB9XG5cbiAgICAudGltZS1jYW5jZWwtYmxvY2stYnV0dG9ucyB7XG4gICAgICBkaXNwbGF5OiBmbGV4O1xuICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xuICAgICAgbWFyZ2luOiAxcmVtIDA7XG4gICAgICB3aWR0aDogMTAwJTtcblxuICAgICAgYnV0dG9uIHtcbiAgICAgICAgZmxleDogMCAxIDI1JTtcbiAgICAgICAgbWFyZ2luOiAwIDAuNXJlbTtcbiAgICAgIH1cbiAgICB9XG4gIH1cblxufVxuXG4ucHJvZ3Jlc3MtYmFyLWNvbnRhaW5lciB7XG4gIHBvc2l0aW9uOiBhYnNvbHV0ZTtcbiAgYm90dG9tOiAwO1xuICBsZWZ0OiAwO1xuICBwYWRkaW5nOiAwIDNyZW07XG4gIHdpZHRoOiAxMDAlO1xuICBoZWlnaHQ6IDNyZW07XG5cbiAgLnByb2dyZXNzLWJhciB7XG4gICAgcG9zaXRpb246IGFic29sdXRlO1xuICAgIHRvcDogLTAuN3JlbTtcbiAgICBsZWZ0OiAwO1xuICAgIG1hcmdpbjogMCAzcmVtO1xuICAgIHdpZHRoOiBjYWxjKDEwMCUgLSA2cmVtKTtcbiAgICBoZWlnaHQ6IDAuN3JlbTtcblxuICAgIC5wcm9ncmVzcy1iYXItZnVsbCB7XG4gICAgICBoZWlnaHQ6IDAuN3JlbTtcbiAgICB9XG4gIH1cblxuICAucHJvZ3Jlc3MtbGFiZWxzIHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG4gICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XG4gICAgZm9udC1zaXplOiAxLjJyZW07XG4gICAgaGVpZ2h0OiAxMDAlO1xuICB9XG5cbiAgLnByb2dyZXNzLXRpbWUge1xuICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcbiAgICB0b3A6IC0zcmVtO1xuICAgIGxlZnQ6IDUwJTtcbiAgICB0cmFuc2Zvcm06IHRyYW5zbGF0ZVgoLTUwJSk7XG4gICAgZm9udC1zaXplOiAxLjJyZW07XG4gIH1cbn1cbiJdfQ== */"

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
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
/* harmony import */ var _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! ../_helpers/pipes/int-to-money.pipe */ "./src/app/_helpers/pipes/int-to-money.pipe.ts");
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
    function PurchaseComponent(route, backend, variablesService, modalService, ngZone, location, intToMoneyPipe) {
        var _this = this;
        this.route = route;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.location = location;
        this.intToMoneyPipe = intToMoneyPipe;
        this.isOpen = false;
        this.localAliases = [];
        this.newPurchase = false;
        this.purchaseForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormGroup"]({
            description: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]('', _angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required),
            seller: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]('', [_angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required, function (g) {
                    if (g.value === _this.variablesService.currentWallet.address) {
                        return { 'address_same': true };
                    }
                    return null;
                }, function (g) {
                    _this.localAliases = [];
                    if (g.value) {
                        if (g.value.indexOf('@') !== 0) {
                            _this.isOpen = false;
                            _this.backend.validateAddress(g.value, function (valid_status) {
                                _this.ngZone.run(function () {
                                    if (valid_status === false) {
                                        g.setErrors(Object.assign({ 'address_not_valid': true }, g.errors));
                                    }
                                    else {
                                        if (g.hasError('address_not_valid')) {
                                            delete g.errors['address_not_valid'];
                                            if (Object.keys(g.errors).length === 0) {
                                                g.setErrors(null);
                                            }
                                        }
                                    }
                                });
                            });
                            return (g.hasError('address_not_valid')) ? { 'address_not_valid': true } : null;
                        }
                        else {
                            _this.isOpen = true;
                            _this.localAliases = _this.variablesService.aliases.filter(function (item) {
                                return item.name.indexOf(g.value) > -1;
                            });
                            if (!(/^@?[a-z0-9\.\-]{6,25}$/.test(g.value))) {
                                g.setErrors(Object.assign({ 'alias_not_valid': true }, g.errors));
                            }
                            else {
                                _this.backend.getAliasByName(g.value.replace('@', ''), function (alias_status, alias_data) {
                                    _this.ngZone.run(function () {
                                        if (alias_status) {
                                            if (alias_data.address === _this.variablesService.currentWallet.address) {
                                                g.setErrors(Object.assign({ 'address_same': true }, g.errors));
                                            }
                                            if (g.hasError('alias_not_valid')) {
                                                delete g.errors['alias_not_valid'];
                                                if (Object.keys(g.errors).length === 0) {
                                                    g.setErrors(null);
                                                }
                                            }
                                        }
                                        else {
                                            g.setErrors(Object.assign({ 'alias_not_valid': true }, g.errors));
                                        }
                                    });
                                });
                            }
                            return (g.hasError('alias_not_valid')) ? { 'alias_not_valid': true } : null;
                        }
                    }
                    return null;
                }]),
            amount: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](null, [_angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required, function (g) {
                    if (parseFloat(g.value) === 0) {
                        return { 'amount_zero': true };
                    }
                    return null;
                }]),
            yourDeposit: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](null, _angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required),
            sellerDeposit: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](null, _angular_forms__WEBPACK_IMPORTED_MODULE_2__["Validators"].required),
            sameAmount: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]({ value: false, disabled: false }),
            comment: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](''),
            fee: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"](this.variablesService.default_fee),
            time: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]({ value: 12, disabled: false }),
            timeCancel: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]({ value: 12, disabled: false }),
            payment: new _angular_forms__WEBPACK_IMPORTED_MODULE_2__["FormControl"]('')
        });
        this.additionalOptions = false;
        this.currentContract = null;
        this.showTimeSelect = false;
        this.showNullify = false;
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
    PurchaseComponent.prototype.addressMouseDown = function (e) {
        if (e['button'] === 0 && this.purchaseForm.get('seller').value && this.purchaseForm.get('seller').value.indexOf('@') === 0) {
            this.isOpen = true;
        }
    };
    PurchaseComponent.prototype.setAlias = function (alias) {
        this.purchaseForm.get('seller').setValue(alias);
    };
    PurchaseComponent.prototype.onClick = function (targetElement) {
        if (targetElement.id !== 'purchase-seller' && this.isOpen) {
            this.isOpen = false;
        }
    };
    PurchaseComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function (params) {
            _this.currentWalletId = params['id'];
        });
        this.subRouting = this.route.params.subscribe(function (params) {
            if (params.hasOwnProperty('id')) {
                _this.currentContract = _this.variablesService.currentWallet.getContract(params['id']);
                _this.purchaseForm.controls['seller'].setValidators([]);
                _this.purchaseForm.updateValueAndValidity();
                _this.purchaseForm.setValue({
                    description: _this.currentContract.private_detailes.t,
                    seller: _this.currentContract.private_detailes.b_addr,
                    amount: _this.intToMoneyPipe.transform(_this.currentContract.private_detailes.to_pay),
                    yourDeposit: _this.intToMoneyPipe.transform(_this.currentContract.private_detailes.a_pledge),
                    sellerDeposit: _this.intToMoneyPipe.transform(_this.currentContract.private_detailes.b_pledge),
                    sameAmount: _this.currentContract.private_detailes.to_pay.isEqualTo(_this.currentContract.private_detailes.b_pledge),
                    comment: _this.currentContract.private_detailes.c,
                    fee: _this.variablesService.default_fee,
                    time: 12,
                    timeCancel: 12,
                    payment: _this.currentContract.payment_id
                });
                _this.purchaseForm.get('sameAmount').disable();
                _this.newPurchase = false;
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
                    setTimeout(function () {
                        _this.variablesService.currentWallet.recountNewContracts();
                    }, 0);
                }
                _this.checkAndChangeHistory();
            }
            else {
                _this.newPurchase = true;
            }
        });
        this.heightAppEvent = this.variablesService.getHeightAppEvent.subscribe(function (newHeight) {
            if (_this.currentContract && _this.currentContract.state === 201 && _this.currentContract.height !== 0 && (newHeight - _this.currentContract.height) >= 10) {
                _this.currentContract.state = 2;
                _this.currentContract.is_new = true;
                _this.variablesService.currentWallet.recountNewContracts();
            }
            else if (_this.currentContract && _this.currentContract.state === 601 && _this.currentContract.height !== 0 && (newHeight - _this.currentContract.height) >= 10) {
                _this.currentContract.state = 6;
                _this.currentContract.is_new = true;
                _this.variablesService.currentWallet.recountNewContracts();
            }
        });
    };
    PurchaseComponent.prototype.toggleOptions = function () {
        this.additionalOptions = !this.additionalOptions;
    };
    PurchaseComponent.prototype.getProgressBarWidth = function () {
        var progress = '0';
        if (!this.newPurchase) {
            if (this.currentContract) {
                if (this.currentContract.state === 1) {
                    progress = '10%';
                }
                if (this.currentContract.state === 201) {
                    progress = '25%';
                }
                if ([120, 2].indexOf(this.currentContract.state) !== -1) {
                    progress = '50%';
                }
                if ([5, 601].indexOf(this.currentContract.state) !== -1) {
                    progress = '75%';
                }
                if ([110, 130, 140, 3, 4, 6].indexOf(this.currentContract.state) !== -1) {
                    progress = '100%';
                }
            }
        }
        return progress;
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
    PurchaseComponent.prototype.createPurchase = function () {
        var _this = this;
        if (this.purchaseForm.valid) {
            var sellerDeposit_1 = this.purchaseForm.get('sameAmount').value ? this.purchaseForm.get('amount').value : this.purchaseForm.get('sellerDeposit').value;
            if (this.purchaseForm.get('seller').value.indexOf('@') !== 0) {
                this.backend.createProposal(this.variablesService.currentWallet.wallet_id, this.purchaseForm.get('description').value, this.purchaseForm.get('comment').value, this.variablesService.currentWallet.address, this.purchaseForm.get('seller').value, this.purchaseForm.get('amount').value, this.purchaseForm.get('yourDeposit').value, sellerDeposit_1, this.purchaseForm.get('time').value, this.purchaseForm.get('payment').value, function (create_status) {
                    if (create_status) {
                        _this.back();
                    }
                });
            }
            else {
                this.backend.getAliasByName(this.purchaseForm.get('seller').value.replace('@', ''), function (alias_status, alias_data) {
                    _this.ngZone.run(function () {
                        if (alias_status === false) {
                            _this.ngZone.run(function () {
                                _this.purchaseForm.get('seller').setErrors({ 'alias_not_valid': true });
                            });
                        }
                        else {
                            _this.backend.createProposal(_this.variablesService.currentWallet.wallet_id, _this.purchaseForm.get('description').value, _this.purchaseForm.get('comment').value, _this.variablesService.currentWallet.address, alias_data.address, _this.purchaseForm.get('amount').value, _this.purchaseForm.get('yourDeposit').value, sellerDeposit_1, _this.purchaseForm.get('time').value, _this.purchaseForm.get('payment').value, function (create_status) {
                                if (create_status) {
                                    _this.back();
                                }
                            });
                        }
                    });
                });
            }
        }
    };
    PurchaseComponent.prototype.back = function () {
        this.location.back();
    };
    PurchaseComponent.prototype.acceptState = function () {
        var _this = this;
        this.backend.acceptProposal(this.currentWalletId, this.currentContract.contract_id, function (accept_status) {
            if (accept_status) {
                _this.modalService.prepareModal('info', 'PURCHASE.ACCEPT_STATE_WAIT_BIG');
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
        this.modalService.prepareModal('info', 'PURCHASE.IGNORED_ACCEPT');
        this.back();
    };
    PurchaseComponent.prototype.productNotGot = function () {
        var _this = this;
        this.backend.releaseProposal(this.currentWalletId, this.currentContract.contract_id, 'REL_B', function (release_status) {
            if (release_status) {
                _this.modalService.prepareModal('info', 'PURCHASE.BURN_PROPOSAL');
                _this.back();
            }
        });
    };
    PurchaseComponent.prototype.dealsDetailsFinish = function () {
        var _this = this;
        this.backend.releaseProposal(this.currentWalletId, this.currentContract.contract_id, 'REL_N', function (release_status) {
            if (release_status) {
                _this.modalService.prepareModal('success', 'PURCHASE.SUCCESS_FINISH_PROPOSAL');
                _this.back();
            }
        });
    };
    PurchaseComponent.prototype.dealsDetailsCancel = function () {
        var _this = this;
        this.backend.requestCancelContract(this.currentWalletId, this.currentContract.contract_id, this.purchaseForm.get('timeCancel').value, function (cancel_status) {
            if (cancel_status) {
                _this.modalService.prepareModal('info', 'PURCHASE.SEND_CANCEL_PROPOSAL');
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
        this.modalService.prepareModal('info', 'PURCHASE.IGNORED_CANCEL');
        this.back();
    };
    PurchaseComponent.prototype.dealsDetailsSellerCancel = function () {
        var _this = this;
        this.backend.acceptCancelContract(this.currentWalletId, this.currentContract.contract_id, function (accept_status) {
            if (accept_status) {
                _this.modalService.prepareModal('info', 'PURCHASE.DEALS_CANCELED_WAIT');
                _this.back();
            }
        });
    };
    PurchaseComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
        this.subRouting.unsubscribe();
        this.heightAppEvent.unsubscribe();
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["HostListener"])('document:click', ['$event.target']),
        __metadata("design:type", Function),
        __metadata("design:paramtypes", [Object]),
        __metadata("design:returntype", void 0)
    ], PurchaseComponent.prototype, "onClick", null);
    PurchaseComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-purchase',
            template: __webpack_require__(/*! ./purchase.component.html */ "./src/app/purchase/purchase.component.html"),
            styles: [__webpack_require__(/*! ./purchase.component.scss */ "./src/app/purchase/purchase.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_1__["ActivatedRoute"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__["VariablesService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__["ModalService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _angular_common__WEBPACK_IMPORTED_MODULE_6__["Location"],
            _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_7__["IntToMoneyPipe"]])
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

module.exports = "<div class=\"wrap-qr\">\n  <img src=\"{{qrImageSrc}}\" alt=\"qr-code\">\n  <div class=\"wrap-address\">\n    <div class=\"address\">{{variablesService.currentWallet.address}}</div>\n    <button type=\"button\" class=\"btn-copy-address\" [class.copy]=\"!copyAnimation\" [class.copied]=\"copyAnimation\" (click)=\"copyAddress()\"></button>\n  </div>\n</div>\n"

/***/ }),

/***/ "./src/app/receive/receive.component.scss":
/*!************************************************!*\
  !*** ./src/app/receive/receive.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.wrap-qr {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  -webkit-box-align: center;\n          align-items: center; }\n\n.wrap-qr img {\n    margin: 4rem 0; }\n\n.wrap-qr .wrap-address {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    font-size: 1.4rem;\n    line-height: 2.7rem; }\n\n.wrap-qr .wrap-address .btn-copy-address {\n      margin-left: 1.2rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.wrap-qr .wrap-address .btn-copy-address.copy {\n        -webkit-mask: url('copy.svg') no-repeat center;\n                mask: url('copy.svg') no-repeat center; }\n\n.wrap-qr .wrap-address .btn-copy-address.copy:hover {\n          opacity: 0.75; }\n\n.wrap-qr .wrap-address .btn-copy-address.copied {\n        -webkit-mask: url('complete-testwallet.svg') no-repeat center;\n                mask: url('complete-testwallet.svg') no-repeat center; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3JlY2VpdmUvcmVjZWl2ZS5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLFdBQVcsRUFBQTs7QUFHYjtFQUNFLG9CQUFhO0VBQWIsYUFBYTtFQUNiLDRCQUFzQjtFQUF0Qiw2QkFBc0I7VUFBdEIsc0JBQXNCO0VBQ3RCLHlCQUFtQjtVQUFuQixtQkFBbUIsRUFBQTs7QUFIckI7SUFNSSxjQUFjLEVBQUE7O0FBTmxCO0lBVUksb0JBQWE7SUFBYixhQUFhO0lBQ2IseUJBQW1CO1lBQW5CLG1CQUFtQjtJQUNuQixpQkFBaUI7SUFDakIsbUJBQW1CLEVBQUE7O0FBYnZCO01BZ0JNLG1CQUFtQjtNQUNuQixhQUFhO01BQ2IsY0FBYyxFQUFBOztBQWxCcEI7UUFxQlEsOENBQXVEO2dCQUF2RCxzQ0FBdUQsRUFBQTs7QUFyQi9EO1VBd0JVLGFBQWEsRUFBQTs7QUF4QnZCO1FBNkJRLDZEQUFzRTtnQkFBdEUscURBQXNFLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9yZWNlaXZlL3JlY2VpdmUuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XG4gIHdpZHRoOiAxMDAlO1xufVxuXG4ud3JhcC1xciB7XG4gIGRpc3BsYXk6IGZsZXg7XG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XG4gIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG5cbiAgaW1nIHtcbiAgICBtYXJnaW46IDRyZW0gMDtcbiAgfVxuXG4gIC53cmFwLWFkZHJlc3Mge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICBmb250LXNpemU6IDEuNHJlbTtcbiAgICBsaW5lLWhlaWdodDogMi43cmVtO1xuXG4gICAgLmJ0bi1jb3B5LWFkZHJlc3Mge1xuICAgICAgbWFyZ2luLWxlZnQ6IDEuMnJlbTtcbiAgICAgIHdpZHRoOiAxLjdyZW07XG4gICAgICBoZWlnaHQ6IDEuN3JlbTtcblxuICAgICAgJi5jb3B5IHtcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9jb3B5LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcblxuICAgICAgICAmOmhvdmVyIHtcbiAgICAgICAgICBvcGFjaXR5OiAwLjc1O1xuICAgICAgICB9XG4gICAgICB9XG5cbiAgICAgICYuY29waWVkIHtcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9jb21wbGV0ZS10ZXN0d2FsbGV0LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgIH1cbiAgICB9XG4gIH1cbn1cbiJdfQ== */"

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
        this.copyAnimation = false;
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
        var _this = this;
        this.backend.setClipboard(this.variablesService.currentWallet.address);
        this.copyAnimation = true;
        this.copyAnimationTimeout = window.setTimeout(function () {
            _this.copyAnimation = false;
        }, 2000);
    };
    ReceiveComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
        clearTimeout(this.copyAnimationTimeout);
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

module.exports = "<div class=\"content\">\n\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span [routerLink]=\"['/main']\">{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\n      <span>{{ 'BREADCRUMBS.RESTORE_WALLET' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" [routerLink]=\"['/main']\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <form class=\"form-restore\" [formGroup]=\"restoreForm\">\n\n    <div class=\"input-block half-block\">\n      <label for=\"wallet-name\">{{ 'RESTORE_WALLET.LABEL_NAME' | translate }}</label>\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [attr.readonly]=\"walletSaved ? '' : null\" [maxLength]=\"variablesService.maxWalletNameLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"restoreForm.controls['name'].invalid && (restoreForm.controls['name'].dirty || restoreForm.controls['name'].touched)\">\n        <div *ngIf=\"restoreForm.controls['name'].errors['required']\">\n          {{ 'RESTORE_WALLET.FORM_ERRORS.NAME_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"restoreForm.controls['name'].errors['duplicate']\">\n          {{ 'RESTORE_WALLET.FORM_ERRORS.NAME_DUPLICATE' | translate }}\n        </div>\n      </div>\n      <div class=\"error-block\" *ngIf=\"restoreForm.get('name').value.length >= variablesService.maxWalletNameLength\">\n        {{ 'RESTORE_WALLET.FORM_ERRORS.MAX_LENGTH' | translate }}\n      </div>\n    </div>\n\n    <div class=\"input-block half-block\">\n      <label for=\"wallet-password\">{{ 'RESTORE_WALLET.PASS' | translate }}</label>\n      <input type=\"password\" id=\"wallet-password\" formControlName=\"password\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\n    </div>\n\n    <div class=\"input-block half-block\">\n      <label for=\"confirm-wallet-password\">{{ 'RESTORE_WALLET.CONFIRM' | translate }}</label>\n      <input type=\"password\" id=\"confirm-wallet-password\" formControlName=\"confirm\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\n      <div class=\"error-block\" *ngIf=\"restoreForm.controls['password'].dirty && restoreForm.controls['confirm'].dirty && restoreForm.errors\">\n        <div *ngIf=\"restoreForm.errors['confirm_mismatch']\">\n          {{ 'RESTORE_WALLET.FORM_ERRORS.CONFIRM_NOT_MATCH' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"input-block\">\n      <label for=\"phrase-key\">{{ 'RESTORE_WALLET.LABEL_PHRASE_KEY' | translate }}</label>\n      <input type=\"text\" id=\"phrase-key\" formControlName=\"key\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"restoreForm.controls['key'].invalid && (restoreForm.controls['key'].dirty || restoreForm.controls['key'].touched)\">\n        <div *ngIf=\"restoreForm.controls['key'].errors['required']\">\n          {{ 'RESTORE_WALLET.FORM_ERRORS.KEY_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"restoreForm.controls['key'].errors['key_not_valid']\">\n          {{ 'RESTORE_WALLET.FORM_ERRORS.KEY_NOT_VALID' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"wrap-buttons\">\n      <button type=\"button\" class=\"transparent-button\" *ngIf=\"walletSaved\" disabled><i class=\"icon\"></i>{{walletSavedName}}</button>\n      <button type=\"button\" class=\"blue-button select-button\" (click)=\"saveWallet()\" [disabled]=\"!restoreForm.valid\" *ngIf=\"!walletSaved\">{{ 'RESTORE_WALLET.BUTTON_SELECT' | translate }}</button>\n      <button type=\"button\" class=\"blue-button create-button\" (click)=\"createWallet()\" [disabled]=\"!walletSaved\">{{ 'RESTORE_WALLET.BUTTON_CREATE' | translate }}</button>\n    </div>\n\n  </form>\n\n</div>\n\n<app-progress-container [width]=\"progressWidth\" [labels]=\"['PROGRESS.ADD_WALLET', 'PROGRESS.SELECT_LOCATION', 'PROGRESS.RESTORE_WALLET']\"></app-progress-container>\n"

/***/ }),

/***/ "./src/app/restore-wallet/restore-wallet.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/restore-wallet/restore-wallet.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: relative; }\n\n.form-restore {\n  margin: 2.4rem 0;\n  width: 100%; }\n\n.form-restore .input-block.half-block {\n    width: 50%; }\n\n.form-restore .wrap-buttons {\n    display: -webkit-box;\n    display: flex;\n    margin: 2.5rem -0.7rem;\n    width: 50%; }\n\n.form-restore .wrap-buttons button {\n      margin: 0 0.7rem; }\n\n.form-restore .wrap-buttons button.transparent-button {\n        flex-basis: 50%; }\n\n.form-restore .wrap-buttons button.select-button {\n        flex-basis: 60%; }\n\n.form-restore .wrap-buttons button.create-button {\n        -webkit-box-flex: 1;\n                flex: 1 1 50%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3Jlc3RvcmUtd2FsbGV0L3Jlc3RvcmUtd2FsbGV0LmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCLEVBQUE7O0FBR3BCO0VBQ0UsZ0JBQWdCO0VBQ2hCLFdBQVcsRUFBQTs7QUFGYjtJQU9NLFVBQVUsRUFBQTs7QUFQaEI7SUFZSSxvQkFBYTtJQUFiLGFBQWE7SUFDYixzQkFBc0I7SUFDdEIsVUFBVSxFQUFBOztBQWRkO01BaUJNLGdCQUFnQixFQUFBOztBQWpCdEI7UUFvQlEsZUFBZSxFQUFBOztBQXBCdkI7UUF3QlEsZUFBZSxFQUFBOztBQXhCdkI7UUE0QlEsbUJBQWE7Z0JBQWIsYUFBYSxFQUFBIiwiZmlsZSI6InNyYy9hcHAvcmVzdG9yZS13YWxsZXQvcmVzdG9yZS13YWxsZXQuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XG4gIHBvc2l0aW9uOiByZWxhdGl2ZTtcbn1cblxuLmZvcm0tcmVzdG9yZSB7XG4gIG1hcmdpbjogMi40cmVtIDA7XG4gIHdpZHRoOiAxMDAlO1xuXG4gIC5pbnB1dC1ibG9jayB7XG5cbiAgICAmLmhhbGYtYmxvY2sge1xuICAgICAgd2lkdGg6IDUwJTtcbiAgICB9XG4gIH1cblxuICAud3JhcC1idXR0b25zIHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIG1hcmdpbjogMi41cmVtIC0wLjdyZW07XG4gICAgd2lkdGg6IDUwJTtcblxuICAgIGJ1dHRvbiB7XG4gICAgICBtYXJnaW46IDAgMC43cmVtO1xuXG4gICAgICAmLnRyYW5zcGFyZW50LWJ1dHRvbiB7XG4gICAgICAgIGZsZXgtYmFzaXM6IDUwJTtcbiAgICAgIH1cblxuICAgICAgJi5zZWxlY3QtYnV0dG9uIHtcbiAgICAgICAgZmxleC1iYXNpczogNjAlO1xuICAgICAgfVxuXG4gICAgICAmLmNyZWF0ZS1idXR0b24ge1xuICAgICAgICBmbGV4OiAxIDEgNTAlO1xuICAgICAgfVxuICAgIH1cbiAgfVxufVxuIl19 */"

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
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! ../_helpers/models/wallet.model */ "./src/app/_helpers/models/wallet.model.ts");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
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
    function RestoreWalletComponent(router, backend, variablesService, modalService, ngZone, translate) {
        var _this = this;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.translate = translate;
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
        this.walletSavedName = '';
        this.progressWidth = '9rem';
    }
    RestoreWalletComponent.prototype.ngOnInit = function () { };
    RestoreWalletComponent.prototype.createWallet = function () {
        var _this = this;
        this.ngZone.run(function () {
            _this.progressWidth = '100%';
            _this.router.navigate(['/seed-phrase'], { queryParams: { wallet_id: _this.wallet.id } });
        });
    };
    RestoreWalletComponent.prototype.saveWallet = function () {
        var _this = this;
        if (this.restoreForm.valid && this.restoreForm.get('name').value.length <= this.variablesService.maxWalletNameLength) {
            this.backend.isValidRestoreWalletText(this.restoreForm.get('key').value, function (valid_status, valid_data) {
                if (valid_data !== 'TRUE') {
                    _this.ngZone.run(function () {
                        _this.restoreForm.get('key').setErrors({ key_not_valid: true });
                    });
                }
                else {
                    _this.backend.saveFileDialog(_this.translate.instant('RESTORE_WALLET.CHOOSE_PATH'), '*', _this.variablesService.settings.default_path, function (save_status, save_data) {
                        if (save_status) {
                            _this.variablesService.settings.default_path = save_data.path.substr(0, save_data.path.lastIndexOf('/'));
                            _this.walletSavedName = save_data.path.substr(save_data.path.lastIndexOf('/') + 1, save_data.path.length - 1);
                            _this.backend.restoreWallet(save_data.path, _this.restoreForm.get('password').value, _this.restoreForm.get('key').value, function (restore_status, restore_data) {
                                if (restore_status) {
                                    _this.wallet.id = restore_data.wallet_id;
                                    _this.variablesService.opening_wallet = new _helpers_models_wallet_model__WEBPACK_IMPORTED_MODULE_6__["Wallet"](restore_data.wallet_id, _this.restoreForm.get('name').value, _this.restoreForm.get('password').value, restore_data['wi'].path, restore_data['wi'].address, restore_data['wi'].balance, restore_data['wi'].unlocked_balance, restore_data['wi'].mined_total, restore_data['wi'].tracking_hey);
                                    _this.variablesService.opening_wallet.alias = _this.backend.getWalletAlias(_this.variablesService.opening_wallet.address);
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
                                        _this.progressWidth = '50%';
                                    });
                                }
                                else {
                                    _this.modalService.prepareModal('error', 'RESTORE_WALLET.NOT_CORRECT_FILE_OR_PASSWORD');
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
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__["ModalService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_7__["TranslateService"]])
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

module.exports = "<div class=\"content\">\n\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span [routerLink]=\"['/main']\">{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\n      <span>{{ 'BREADCRUMBS.SAVE_PHRASE' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <h3 class=\"seed-phrase-title\">{{ 'SEED_PHRASE.TITLE' | translate }}</h3>\n\n  <div class=\"seed-phrase-content\" (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, seedPhrase)\">\n    <ng-container *ngFor=\"let word of seedPhrase.split(' '); let index = index\">\n      <div class=\"word\">{{(index + 1) + '. ' + word}}</div>\n    </ng-container>\n  </div>\n\n  <div class=\"wrap-buttons\">\n    <button type=\"button\" class=\"blue-button seed-phrase-button\" (click)=\"runWallet()\">{{ 'SEED_PHRASE.BUTTON_CREATE_ACCOUNT' | translate }}</button>\n    <button type=\"button\" class=\"blue-button copy-button\" *ngIf=\"!seedPhraseCopied\" (click)=\"copySeedPhrase()\">{{ 'SEED_PHRASE.BUTTON_COPY' | translate }}</button>\n    <button type=\"button\" class=\"transparent-button copy-button\" *ngIf=\"seedPhraseCopied\" disabled><i class=\"icon\"></i>{{ 'SEED_PHRASE.BUTTON_COPY' | translate }}</button>\n  </div>\n</div>\n\n<app-progress-container [width]=\"'100%'\" [labels]=\"['PROGRESS.ADD_WALLET', 'PROGRESS.SELECT_LOCATION', 'PROGRESS.CREATE_WALLET']\"></app-progress-container>\n"

/***/ }),

/***/ "./src/app/seed-phrase/seed-phrase.component.scss":
/*!********************************************************!*\
  !*** ./src/app/seed-phrase/seed-phrase.component.scss ***!
  \********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: relative; }\n\n.seed-phrase-title {\n  line-height: 2.2rem;\n  padding: 2.2rem 0; }\n\n.seed-phrase-content {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  flex-wrap: wrap;\n  padding: 1.4rem;\n  width: 100%;\n  height: 12rem; }\n\n.seed-phrase-content .word {\n    line-height: 2.2rem;\n    max-width: 13rem; }\n\n.wrap-buttons {\n  display: -webkit-box;\n  display: flex; }\n\n.wrap-buttons .seed-phrase-button {\n    margin: 2.8rem 0;\n    width: 25%;\n    min-width: 1.5rem; }\n\n.wrap-buttons .copy-button {\n    margin: 2.8rem 1rem;\n    width: 25%;\n    min-width: 1.5rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3NlZWQtcGhyYXNlL3NlZWQtcGhyYXNlLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCLEVBQUE7O0FBR3BCO0VBQ0UsbUJBQW1CO0VBQ25CLGlCQUFpQixFQUFBOztBQUduQjtFQUNFLG9CQUFhO0VBQWIsYUFBYTtFQUNiLDRCQUFzQjtFQUF0Qiw2QkFBc0I7VUFBdEIsc0JBQXNCO0VBQ3RCLGVBQWU7RUFDZixlQUFlO0VBQ2YsV0FBVztFQUNYLGFBQWEsRUFBQTs7QUFOZjtJQVNJLG1CQUFtQjtJQUNuQixnQkFBZ0IsRUFBQTs7QUFJcEI7RUFDRSxvQkFBYTtFQUFiLGFBQWEsRUFBQTs7QUFEZjtJQUlJLGdCQUFnQjtJQUNoQixVQUFVO0lBQ1YsaUJBQWlCLEVBQUE7O0FBTnJCO0lBVUksbUJBQW1CO0lBQ25CLFVBQVU7SUFDVixpQkFBaUIsRUFBQSIsImZpbGUiOiJzcmMvYXBwL3NlZWQtcGhyYXNlL3NlZWQtcGhyYXNlLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xuICBwb3NpdGlvbjogcmVsYXRpdmU7XG59XG5cbi5zZWVkLXBocmFzZS10aXRsZSB7XG4gIGxpbmUtaGVpZ2h0OiAyLjJyZW07XG4gIHBhZGRpbmc6IDIuMnJlbSAwO1xufVxuXG4uc2VlZC1waHJhc2UtY29udGVudCB7XG4gIGRpc3BsYXk6IGZsZXg7XG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XG4gIGZsZXgtd3JhcDogd3JhcDtcbiAgcGFkZGluZzogMS40cmVtO1xuICB3aWR0aDogMTAwJTtcbiAgaGVpZ2h0OiAxMnJlbTtcblxuICAud29yZCB7XG4gICAgbGluZS1oZWlnaHQ6IDIuMnJlbTtcbiAgICBtYXgtd2lkdGg6IDEzcmVtO1xuICB9XG59XG5cbi53cmFwLWJ1dHRvbnMge1xuICBkaXNwbGF5OiBmbGV4O1xuXG4gIC5zZWVkLXBocmFzZS1idXR0b24ge1xuICAgIG1hcmdpbjogMi44cmVtIDA7XG4gICAgd2lkdGg6IDI1JTtcbiAgICBtaW4td2lkdGg6IDEuNXJlbTtcbiAgfVxuXG4gIC5jb3B5LWJ1dHRvbiB7XG4gICAgbWFyZ2luOiAyLjhyZW0gMXJlbTtcbiAgICB3aWR0aDogMjUlO1xuICAgIG1pbi13aWR0aDogMS41cmVtO1xuICB9XG59XG5cbiJdfQ== */"

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
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
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
    function SeedPhraseComponent(route, router, location, backend, variablesService, modalService, ngZone) {
        this.route = route;
        this.router = router;
        this.location = location;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.seedPhrase = '';
        this.seedPhraseCopied = false;
    }
    SeedPhraseComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.queryRouting = this.route.queryParams.subscribe(function (params) {
            if (params.wallet_id) {
                _this.wallet_id = params.wallet_id;
                _this.backend.getSmartWalletInfo(params.wallet_id, function (status, data) {
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
                    if (_this.variablesService.appPass) {
                        _this.backend.storeSecureAppData();
                    }
                    _this.ngZone.run(function () {
                        _this.router.navigate(['/wallet/' + _this.wallet_id]);
                    });
                }
                else {
                    console.log(run_data['error_code']);
                }
            });
        }
        else {
            this.variablesService.opening_wallet = null;
            this.modalService.prepareModal('error', 'OPEN_WALLET.WITH_ADDRESS_ALREADY_OPEN');
            this.backend.closeWallet(this.wallet_id, function () {
                _this.ngZone.run(function () {
                    _this.router.navigate(['/']);
                });
            });
        }
    };
    SeedPhraseComponent.prototype.copySeedPhrase = function () {
        var _this = this;
        this.backend.setClipboard(this.seedPhrase, function () {
            _this.ngZone.run(function () {
                _this.seedPhraseCopied = true;
            });
        });
    };
    SeedPhraseComponent.prototype.back = function () {
        this.location.back();
    };
    SeedPhraseComponent.prototype.ngOnDestroy = function () {
        this.queryRouting.unsubscribe();
    };
    SeedPhraseComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-seed-phrase',
            template: __webpack_require__(/*! ./seed-phrase.component.html */ "./src/app/seed-phrase/seed-phrase.component.html"),
            styles: [__webpack_require__(/*! ./seed-phrase.component.scss */ "./src/app/seed-phrase/seed-phrase.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_3__["ActivatedRoute"],
            _angular_router__WEBPACK_IMPORTED_MODULE_3__["Router"],
            _angular_common__WEBPACK_IMPORTED_MODULE_1__["Location"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__["VariablesService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__["ModalService"],
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

module.exports = "<form class=\"form-send\" [formGroup]=\"sendForm\" (ngSubmit)=\"onSend()\">\n\n  <div class=\"input-block input-block-alias\">\n    <label for=\"send-address\">{{ 'SEND.ADDRESS' | translate }}</label>\n\n    <input type=\"text\" id=\"send-address\" formControlName=\"address\" (mousedown)=\"addressMouseDown($event)\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n\n    <div class=\"alias-dropdown scrolled-content\" *ngIf=\"isOpen\">\n      <div *ngFor=\"let item of localAliases\" (click)=\"setAlias(item.name)\">{{item.name}}</div>\n    </div>\n\n    <div class=\"error-block\" *ngIf=\"sendForm.controls['address'].invalid && (sendForm.controls['address'].dirty || sendForm.controls['address'].touched)\">\n      <div *ngIf=\"sendForm.controls['address'].errors['required']\">\n        {{ 'SEND.FORM_ERRORS.ADDRESS_REQUIRED' | translate }}\n      </div>\n      <div *ngIf=\"sendForm.controls['address'].errors['address_not_valid']\">\n        {{ 'SEND.FORM_ERRORS.ADDRESS_NOT_VALID' | translate }}\n      </div>\n      <div *ngIf=\"sendForm.controls['address'].errors['alias_not_valid']\">\n        {{ 'SEND.FORM_ERRORS.ALIAS_NOT_VALID' | translate }}\n      </div>\n    </div>\n  </div>\n\n  <div class=\"input-blocks-row\">\n\n    <div class=\"input-block\">\n      <label for=\"send-amount\">{{ 'SEND.AMOUNT' | translate }}</label>\n      <input type=\"text\" id=\"send-amount\" formControlName=\"amount\" appInputValidate=\"money\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"sendForm.controls['amount'].invalid && (sendForm.controls['amount'].dirty || sendForm.controls['amount'].touched)\">\n        <div *ngIf=\"sendForm.controls['amount'].errors['required']\">\n          {{ 'SEND.FORM_ERRORS.AMOUNT_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"sendForm.controls['amount'].errors['zero']\">\n          {{ 'SEND.FORM_ERRORS.AMOUNT_ZERO' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"input-block\">\n      <label for=\"send-comment\">{{ 'SEND.COMMENT' | translate }}</label>\n      <input type=\"text\" id=\"send-comment\" formControlName=\"comment\" [maxLength]=\"variablesService.maxCommentLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"sendForm.get('comment').value && sendForm.get('comment').value.length >= variablesService.maxCommentLength\">\n        {{ 'SEND.FORM_ERRORS.MAX_LENGTH' | translate }}\n      </div>\n    </div>\n\n  </div>\n\n  <button type=\"button\" class=\"send-select\" (click)=\"toggleOptions()\">\n    <span>{{ 'SEND.DETAILS' | translate }}</span><i class=\"icon arrow\" [class.down]=\"!additionalOptions\" [class.up]=\"additionalOptions\"></i>\n  </button>\n\n  <div class=\"additional-details\" *ngIf=\"additionalOptions\">\n\n    <div class=\"input-block\">\n      <label for=\"send-mixin\">{{ 'SEND.MIXIN' | translate }}</label>\n      <input type=\"text\" id=\"send-mixin\" formControlName=\"mixin\" appInputValidate=\"integer\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"sendForm.controls['mixin'].invalid && (sendForm.controls['mixin'].dirty || sendForm.controls['mixin'].touched)\">\n        <div *ngIf=\"sendForm.controls['mixin'].errors['required']\">\n          {{ 'SEND.FORM_ERRORS.AMOUNT_REQUIRED' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"input-block\">\n      <label for=\"send-fee\">{{ 'SEND.FEE' | translate }}</label>\n      <input type=\"text\" id=\"send-fee\" formControlName=\"fee\" appInputValidate=\"money\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"sendForm.controls['fee'].invalid && (sendForm.controls['fee'].dirty || sendForm.controls['fee'].touched)\">\n        <div *ngIf=\"sendForm.controls['fee'].errors['required']\">\n          {{ 'SEND.FORM_ERRORS.FEE_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"sendForm.controls['fee'].errors['less_min']\">\n          {{ 'SEND.FORM_ERRORS.FEE_MINIMUM' | translate : {fee: variablesService.default_fee} }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"checkbox-block\">\n      <input type=\"checkbox\" id=\"send-hide\" class=\"style-checkbox\" formControlName=\"hide\">\n      <label for=\"send-hide\">{{ 'SEND.HIDE' | translate }}</label>\n    </div>\n\n  </div>\n\n  <button type=\"submit\" class=\"blue-button\" [disabled]=\"!sendForm.valid || !variablesService.currentWallet.loaded\">{{ 'SEND.BUTTON' | translate }}</button>\n\n</form>\n"

/***/ }),

/***/ "./src/app/send/send.component.scss":
/*!******************************************!*\
  !*** ./src/app/send/send.component.scss ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.form-send .input-blocks-row {\n  display: -webkit-box;\n  display: flex; }\n\n.form-send .input-blocks-row > div {\n    flex-basis: 50%; }\n\n.form-send .input-blocks-row > div:first-child {\n      margin-right: 1.5rem; }\n\n.form-send .input-blocks-row > div:last-child {\n      margin-left: 1.5rem; }\n\n.form-send .send-select {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  background: transparent;\n  border: none;\n  font-size: 1.3rem;\n  line-height: 1.3rem;\n  margin: 1.5rem 0 0;\n  padding: 0;\n  width: 100%;\n  max-width: 15rem;\n  height: 1.3rem; }\n\n.form-send .send-select .arrow {\n    margin-left: 1rem;\n    width: 0.8rem;\n    height: 0.8rem; }\n\n.form-send .send-select .arrow.down {\n      -webkit-mask: url('arrow-down.svg') no-repeat center;\n              mask: url('arrow-down.svg') no-repeat center; }\n\n.form-send .send-select .arrow.up {\n      -webkit-mask: url('arrow-up.svg') no-repeat center;\n              mask: url('arrow-up.svg') no-repeat center; }\n\n.form-send .additional-details {\n  display: -webkit-box;\n  display: flex;\n  margin-top: 1.5rem;\n  padding: 0.5rem 0 2rem; }\n\n.form-send .additional-details > div {\n    flex-basis: 25%; }\n\n.form-send .additional-details > div:first-child {\n      padding-left: 1.5rem;\n      padding-right: 1rem; }\n\n.form-send .additional-details > div:last-child {\n      padding-left: 1rem;\n      padding-right: 1.5rem; }\n\n.form-send .additional-details .checkbox-block {\n    flex-basis: 50%; }\n\n.form-send .additional-details .checkbox-block > label {\n      top: 3.5rem; }\n\n.form-send button {\n  margin: 2.4rem 0;\n  width: 100%;\n  max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3NlbmQvc2VuZC5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLFdBQVcsRUFBQTs7QUFHYjtFQUdJLG9CQUFhO0VBQWIsYUFBYSxFQUFBOztBQUhqQjtJQU1NLGVBQWUsRUFBQTs7QUFOckI7TUFTUSxvQkFBb0IsRUFBQTs7QUFUNUI7TUFhUSxtQkFBbUIsRUFBQTs7QUFiM0I7RUFtQkksb0JBQWE7RUFBYixhQUFhO0VBQ2IseUJBQW1CO1VBQW5CLG1CQUFtQjtFQUNuQix1QkFBdUI7RUFDdkIsWUFBWTtFQUNaLGlCQUFpQjtFQUNqQixtQkFBbUI7RUFDbkIsa0JBQWtCO0VBQ2xCLFVBQVU7RUFDVixXQUFXO0VBQ1gsZ0JBQWdCO0VBQ2hCLGNBQWMsRUFBQTs7QUE3QmxCO0lBZ0NNLGlCQUFpQjtJQUNqQixhQUFhO0lBQ2IsY0FBYyxFQUFBOztBQWxDcEI7TUFxQ1Esb0RBQTREO2NBQTVELDRDQUE0RCxFQUFBOztBQXJDcEU7TUF5Q1Esa0RBQTBEO2NBQTFELDBDQUEwRCxFQUFBOztBQXpDbEU7RUErQ0ksb0JBQWE7RUFBYixhQUFhO0VBQ2Isa0JBQWtCO0VBQ2xCLHNCQUFzQixFQUFBOztBQWpEMUI7SUFvRE0sZUFBZSxFQUFBOztBQXBEckI7TUF1RFEsb0JBQW9CO01BQ3BCLG1CQUFtQixFQUFBOztBQXhEM0I7TUE0RFEsa0JBQWtCO01BQ2xCLHFCQUFxQixFQUFBOztBQTdEN0I7SUFrRU0sZUFBZSxFQUFBOztBQWxFckI7TUFxRVEsV0FBVyxFQUFBOztBQXJFbkI7RUEyRUksZ0JBQWdCO0VBQ2hCLFdBQVc7RUFDWCxnQkFBZ0IsRUFBQSIsImZpbGUiOiJzcmMvYXBwL3NlbmQvc2VuZC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcbiAgd2lkdGg6IDEwMCU7XG59XG5cbi5mb3JtLXNlbmQge1xuXG4gIC5pbnB1dC1ibG9ja3Mtcm93IHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuXG4gICAgPiBkaXYge1xuICAgICAgZmxleC1iYXNpczogNTAlO1xuXG4gICAgICAmOmZpcnN0LWNoaWxkIHtcbiAgICAgICAgbWFyZ2luLXJpZ2h0OiAxLjVyZW07XG4gICAgICB9XG5cbiAgICAgICY6bGFzdC1jaGlsZCB7XG4gICAgICAgIG1hcmdpbi1sZWZ0OiAxLjVyZW07XG4gICAgICB9XG4gICAgfVxuICB9XG5cbiAgLnNlbmQtc2VsZWN0IHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG4gICAgYmFja2dyb3VuZDogdHJhbnNwYXJlbnQ7XG4gICAgYm9yZGVyOiBub25lO1xuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xuICAgIGxpbmUtaGVpZ2h0OiAxLjNyZW07XG4gICAgbWFyZ2luOiAxLjVyZW0gMCAwO1xuICAgIHBhZGRpbmc6IDA7XG4gICAgd2lkdGg6IDEwMCU7XG4gICAgbWF4LXdpZHRoOiAxNXJlbTtcbiAgICBoZWlnaHQ6IDEuM3JlbTtcblxuICAgIC5hcnJvdyB7XG4gICAgICBtYXJnaW4tbGVmdDogMXJlbTtcbiAgICAgIHdpZHRoOiAwLjhyZW07XG4gICAgICBoZWlnaHQ6IDAuOHJlbTtcblxuICAgICAgJi5kb3duIHtcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL2Fycm93LWRvd24uc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xuICAgICAgfVxuXG4gICAgICAmLnVwIHtcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL2Fycm93LXVwLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgIH1cbiAgICB9XG4gIH1cblxuICAuYWRkaXRpb25hbC1kZXRhaWxzIHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIG1hcmdpbi10b3A6IDEuNXJlbTtcbiAgICBwYWRkaW5nOiAwLjVyZW0gMCAycmVtO1xuXG4gICAgPiBkaXYge1xuICAgICAgZmxleC1iYXNpczogMjUlO1xuXG4gICAgICAmOmZpcnN0LWNoaWxkIHtcbiAgICAgICAgcGFkZGluZy1sZWZ0OiAxLjVyZW07XG4gICAgICAgIHBhZGRpbmctcmlnaHQ6IDFyZW07XG4gICAgICB9XG5cbiAgICAgICY6bGFzdC1jaGlsZCB7XG4gICAgICAgIHBhZGRpbmctbGVmdDogMXJlbTtcbiAgICAgICAgcGFkZGluZy1yaWdodDogMS41cmVtO1xuICAgICAgfVxuICAgIH1cblxuICAgIC5jaGVja2JveC1ibG9jayB7XG4gICAgICBmbGV4LWJhc2lzOiA1MCU7XG5cbiAgICAgID4gbGFiZWwge1xuICAgICAgICB0b3A6IDMuNXJlbTtcbiAgICAgIH1cbiAgICB9XG4gIH1cblxuICBidXR0b24ge1xuICAgIG1hcmdpbjogMi40cmVtIDA7XG4gICAgd2lkdGg6IDEwMCU7XG4gICAgbWF4LXdpZHRoOiAxNXJlbTtcbiAgfVxufVxuIl19 */"

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
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! bignumber.js */ "./node_modules/bignumber.js/bignumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_6___default = /*#__PURE__*/__webpack_require__.n(bignumber_js__WEBPACK_IMPORTED_MODULE_6__);
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
    function SendComponent(route, backend, variablesService, modalService, ngZone) {
        var _this = this;
        this.route = route;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.isOpen = false;
        this.localAliases = [];
        this.currentWalletId = null;
        this.sendForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            address: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [_angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required, function (g) {
                    _this.localAliases = [];
                    if (g.value) {
                        if (g.value.indexOf('@') !== 0) {
                            _this.isOpen = false;
                            _this.backend.validateAddress(g.value, function (valid_status) {
                                _this.ngZone.run(function () {
                                    if (valid_status === false) {
                                        g.setErrors(Object.assign({ 'address_not_valid': true }, g.errors));
                                    }
                                    else {
                                        if (g.hasError('address_not_valid')) {
                                            delete g.errors['address_not_valid'];
                                            if (Object.keys(g.errors).length === 0) {
                                                g.setErrors(null);
                                            }
                                        }
                                    }
                                });
                            });
                            return (g.hasError('address_not_valid')) ? { 'address_not_valid': true } : null;
                        }
                        else {
                            _this.isOpen = true;
                            _this.localAliases = _this.variablesService.aliases.filter(function (item) {
                                return item.name.indexOf(g.value) > -1;
                            });
                            if (!(/^@?[a-z0-9\.\-]{6,25}$/.test(g.value))) {
                                g.setErrors(Object.assign({ 'alias_not_valid': true }, g.errors));
                            }
                            else {
                                _this.backend.getAliasByName(g.value.replace('@', ''), function (alias_status) {
                                    _this.ngZone.run(function () {
                                        if (alias_status) {
                                            if (g.hasError('alias_not_valid')) {
                                                delete g.errors['alias_not_valid'];
                                                if (Object.keys(g.errors).length === 0) {
                                                    g.setErrors(null);
                                                }
                                            }
                                        }
                                        else {
                                            g.setErrors(Object.assign({ 'alias_not_valid': true }, g.errors));
                                        }
                                    });
                                });
                            }
                            return (g.hasError('alias_not_valid')) ? { 'alias_not_valid': true } : null;
                        }
                    }
                    return null;
                }]),
            amount: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](null, [_angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required, function (g) {
                    if (new bignumber_js__WEBPACK_IMPORTED_MODULE_6__["BigNumber"](g.value).eq(0)) {
                        return { 'zero': true };
                    }
                    return null;
                }]),
            comment: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [function (g) {
                    if (g.value > _this.variablesService.maxCommentLength) {
                        return { 'maxLength': true };
                    }
                    else {
                        return null;
                    }
                }]),
            mixin: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](0, _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required),
            fee: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](this.variablesService.default_fee, [_angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required, function (g) {
                    if ((new bignumber_js__WEBPACK_IMPORTED_MODULE_6__["BigNumber"](g.value)).isLessThan(_this.variablesService.default_fee)) {
                        return { 'less_min': true };
                    }
                    return null;
                }]),
            hide: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](false)
        });
        this.additionalOptions = false;
    }
    SendComponent.prototype.addressMouseDown = function (e) {
        if (e['button'] === 0 && this.sendForm.get('address').value && this.sendForm.get('address').value.indexOf('@') === 0) {
            this.isOpen = true;
        }
    };
    SendComponent.prototype.setAlias = function (alias) {
        this.sendForm.get('address').setValue(alias);
    };
    SendComponent.prototype.onClick = function (targetElement) {
        if (targetElement.id !== 'send-address' && this.isOpen) {
            this.isOpen = false;
        }
    };
    SendComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function (params) {
            _this.currentWalletId = params['id'];
            _this.sendForm.reset({
                address: _this.variablesService.currentWallet.send_data['address'],
                amount: _this.variablesService.currentWallet.send_data['amount'],
                comment: _this.variablesService.currentWallet.send_data['comment'],
                mixin: _this.variablesService.currentWallet.send_data['mixin'] || 0,
                fee: _this.variablesService.currentWallet.send_data['fee'] || _this.variablesService.default_fee,
                hide: _this.variablesService.currentWallet.send_data['hide'] || false
            });
        });
    };
    SendComponent.prototype.onSend = function () {
        var _this = this;
        if (this.sendForm.valid) {
            if (this.sendForm.get('address').value.indexOf('@') !== 0) {
                this.backend.validateAddress(this.sendForm.get('address').value, function (valid_status) {
                    if (valid_status === false) {
                        _this.ngZone.run(function () {
                            _this.sendForm.get('address').setErrors({ 'address_not_valid': true });
                        });
                    }
                    else {
                        _this.backend.sendMoney(_this.currentWalletId, _this.sendForm.get('address').value, _this.sendForm.get('amount').value, _this.sendForm.get('fee').value, _this.sendForm.get('mixin').value, _this.sendForm.get('comment').value, _this.sendForm.get('hide').value, function (send_status) {
                            if (send_status) {
                                _this.modalService.prepareModal('success', 'SEND.SUCCESS_SENT');
                                _this.variablesService.currentWallet.send_data = { address: null, amount: null, comment: null, mixin: null, fee: null, hide: null };
                                _this.sendForm.reset({ address: null, amount: null, comment: null, mixin: 0, fee: _this.variablesService.default_fee, hide: false });
                            }
                        });
                    }
                });
            }
            else {
                this.backend.getAliasByName(this.sendForm.get('address').value.replace('@', ''), function (alias_status, alias_data) {
                    _this.ngZone.run(function () {
                        if (alias_status === false) {
                            _this.ngZone.run(function () {
                                _this.sendForm.get('address').setErrors({ 'alias_not_valid': true });
                            });
                        }
                        else {
                            _this.backend.sendMoney(_this.currentWalletId, alias_data.address, // this.sendForm.get('address').value,
                            _this.sendForm.get('amount').value, _this.sendForm.get('fee').value, _this.sendForm.get('mixin').value, _this.sendForm.get('comment').value, _this.sendForm.get('hide').value, function (send_status) {
                                if (send_status) {
                                    _this.modalService.prepareModal('success', 'SEND.SUCCESS_SENT');
                                    _this.variablesService.currentWallet.send_data = { address: null, amount: null, comment: null, mixin: null, fee: null, hide: null };
                                    _this.sendForm.reset({ address: null, amount: null, comment: null, mixin: 0, fee: _this.variablesService.default_fee, hide: false });
                                }
                            });
                        }
                    });
                });
            }
        }
    };
    SendComponent.prototype.toggleOptions = function () {
        this.additionalOptions = !this.additionalOptions;
    };
    SendComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
        this.variablesService.currentWallet.send_data = {
            address: this.sendForm.get('address').value,
            amount: this.sendForm.get('amount').value,
            comment: this.sendForm.get('comment').value,
            mixin: this.sendForm.get('mixin').value,
            fee: this.sendForm.get('fee').value,
            hide: this.sendForm.get('hide').value
        };
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["HostListener"])('document:click', ['$event.target']),
        __metadata("design:type", Function),
        __metadata("design:paramtypes", [Object]),
        __metadata("design:returntype", void 0)
    ], SendComponent.prototype, "onClick", null);
    SendComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-send',
            template: __webpack_require__(/*! ./send.component.html */ "./src/app/send/send.component.html"),
            styles: [__webpack_require__(/*! ./send.component.scss */ "./src/app/send/send.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_2__["ActivatedRoute"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__["VariablesService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__["ModalService"],
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

module.exports = "<div class=\"content scrolled-content\">\n\n  <div>\n    <div class=\"head\">\n      <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n        <i class=\"icon back\"></i>\n        <span>{{ 'COMMON.BACK' | translate }}</span>\n      </button>\n    </div>\n\n    <h3 class=\"settings-title\">{{ 'SETTINGS.TITLE' | translate }}</h3>\n\n    <div class=\"theme-selection\">\n      <div class=\"radio-block\">\n        <input class=\"style-radio\" type=\"radio\" id=\"dark\" name=\"theme\" value=\"dark\" [checked]=\"theme == 'dark'\" (change)=\"setTheme('dark')\">\n        <label for=\"dark\">{{ 'SETTINGS.DARK_THEME' | translate }}</label>\n      </div>\n      <div class=\"radio-block\">\n        <input class=\"style-radio\" type=\"radio\" id=\"white\" name=\"theme\" value=\"white\" [checked]=\"theme == 'white'\" (change)=\"setTheme('white')\">\n        <label for=\"white\">{{ 'SETTINGS.WHITE_THEME' | translate }}</label>\n      </div>\n      <div class=\"radio-block\">\n        <input class=\"style-radio\" type=\"radio\" id=\"gray\" name=\"theme\" value=\"gray\" [checked]=\"theme == 'gray'\" (change)=\"setTheme('gray')\">\n        <label for=\"gray\">{{ 'SETTINGS.GRAY_THEME' | translate }}</label>\n      </div>\n    </div>\n\n    <div class=\"scale-selection\">\n      <button type=\"button\" class=\"button-block\" [class.active]=\"item.id === variablesService.settings.scale\" *ngFor=\"let item of appScaleOptions\" (click)=\"setScale(item.id)\">\n        <span class=\"label\">{{item.name}}</span>\n      </button>\n    </div>\n\n    <div class=\"lock-selection\">\n      <label class=\"lock-selection-title\">{{ 'SETTINGS.APP_LOCK.TITLE' | translate }}</label>\n      <ng-select class=\"custom-select\"\n                 [items]=\"appLockOptions\"\n                 bindValue=\"id\"\n                 bindLabel=\"name\"\n                 [(ngModel)]=\"variablesService.settings.appLockTime\"\n                 [clearable]=\"false\"\n                 [searchable]=\"false\"\n                 (change)=\"onLockChange()\">\n        <ng-template ng-label-tmp let-item=\"item\">\n          {{item.name | translate}}\n        </ng-template>\n        <ng-template ng-option-tmp let-item=\"item\" let-index=\"index\">\n          {{item.name | translate}}\n        </ng-template>\n      </ng-select>\n    </div>\n\n    <div class=\"lock-selection\">\n      <label class=\"lock-selection-title\">{{ 'SETTINGS.APP_LOG_TITLE' | translate }}</label>\n      <ng-select class=\"custom-select\"\n                 [items]=\"appLogOptions\"\n                 bindValue=\"id\"\n                 bindLabel=\"id\"\n                 [(ngModel)]=\"variablesService.settings.appLog\"\n                 [clearable]=\"false\"\n                 [searchable]=\"false\"\n                 (change)=\"onLogChange()\">\n      </ng-select>\n    </div>\n\n    <form class=\"master-password\" [formGroup]=\"changeForm\" (ngSubmit)=\"onSubmitChangePass()\">\n\n      <span class=\"master-password-title\">{{ 'SETTINGS.MASTER_PASSWORD.TITLE' | translate }}</span>\n\n      <div class=\"input-block\" *ngIf=\"variablesService.appPass\">\n        <label for=\"old-password\">{{ 'SETTINGS.MASTER_PASSWORD.OLD' | translate }}</label>\n        <input type=\"password\" id=\"old-password\" formControlName=\"password\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\"/>\n        <div class=\"error-block\" *ngIf=\"changeForm.invalid && changeForm.controls['password'].valid && (changeForm.controls['password'].dirty || changeForm.controls['password'].touched) && changeForm.errors && changeForm.errors.pass_mismatch\">\n          {{ 'SETTINGS.FORM_ERRORS.PASS_NOT_MATCH' | translate }}\n        </div>\n      </div>\n\n      <div class=\"input-block\">\n        <label for=\"new-password\">{{ 'SETTINGS.MASTER_PASSWORD.NEW' | translate }}</label>\n        <input type=\"password\" id=\"new-password\" formControlName=\"new_password\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\"/>\n      </div>\n\n      <div class=\"input-block\">\n        <label for=\"confirm-password\">{{ 'SETTINGS.MASTER_PASSWORD.CONFIRM' | translate }}</label>\n        <input type=\"password\" id=\"confirm-password\" formControlName=\"new_confirmation\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\"/>\n        <div class=\"error-block\" *ngIf=\"changeForm.invalid && (changeForm.controls['new_confirmation'].dirty || changeForm.controls['new_confirmation'].touched) && changeForm.errors && changeForm.errors.confirm_mismatch\">\n          {{ 'SETTINGS.FORM_ERRORS.CONFIRM_NOT_MATCH' | translate }}\n        </div>\n      </div>\n\n      <button type=\"submit\" class=\"blue-button\" [disabled]=\"!changeForm.valid\">{{ 'SETTINGS.MASTER_PASSWORD.BUTTON' | translate }}</button>\n\n    </form>\n  </div>\n\n  <div>\n    <div class=\"last-build\">{{ 'SETTINGS.LAST_BUILD' | translate : {value: currentBuild} }}</div>\n  </div>\n\n</div>\n"

/***/ }),

/***/ "./src/app/settings/settings.component.scss":
/*!**************************************************!*\
  !*** ./src/app/settings/settings.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".head {\n  -webkit-box-pack: end;\n          justify-content: flex-end; }\n\n.settings-title {\n  font-size: 1.7rem; }\n\n.theme-selection {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  -webkit-box-align: start;\n          align-items: flex-start;\n  margin: 2.4rem 0;\n  width: 50%; }\n\n.theme-selection .radio-block {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: start;\n            justify-content: flex-start;\n    font-size: 1.3rem;\n    line-height: 2.7rem; }\n\n.lock-selection {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  -webkit-box-align: start;\n          align-items: flex-start;\n  margin: 2.4rem 0;\n  width: 50%; }\n\n.lock-selection .lock-selection-title {\n    display: -webkit-box;\n    display: flex;\n    font-size: 1.5rem;\n    line-height: 2.7rem;\n    margin-bottom: 1rem; }\n\n.scale-selection {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  -webkit-box-pack: justify;\n          justify-content: space-between;\n  padding: 0 0 4rem;\n  width: 50%;\n  height: 0.5rem; }\n\n.scale-selection .button-block {\n    position: relative;\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: center;\n            justify-content: center;\n    -webkit-box-flex: 1;\n            flex: 1 0 auto;\n    margin: 0 0.2rem;\n    padding: 0;\n    height: 0.5rem; }\n\n.scale-selection .button-block .label {\n      position: absolute;\n      bottom: -1rem;\n      left: 50%;\n      -webkit-transform: translate(-50%, 100%);\n              transform: translate(-50%, 100%);\n      font-size: 1rem;\n      white-space: nowrap; }\n\n.master-password {\n  width: 50%; }\n\n.master-password .master-password-title {\n    display: -webkit-box;\n    display: flex;\n    font-size: 1.5rem;\n    line-height: 2.7rem;\n    margin-bottom: 1rem; }\n\n.master-password button {\n    margin: 2.5rem auto;\n    width: 100%;\n    max-width: 15rem; }\n\n.last-build {\n  font-size: 1rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3NldHRpbmdzL3NldHRpbmdzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UscUJBQXlCO1VBQXpCLHlCQUF5QixFQUFBOztBQUczQjtFQUNFLGlCQUFpQixFQUFBOztBQUduQjtFQUNFLG9CQUFhO0VBQWIsYUFBYTtFQUNiLDRCQUFzQjtFQUF0Qiw2QkFBc0I7VUFBdEIsc0JBQXNCO0VBQ3RCLHdCQUF1QjtVQUF2Qix1QkFBdUI7RUFDdkIsZ0JBQWdCO0VBQ2hCLFVBQVUsRUFBQTs7QUFMWjtJQVFJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLHlCQUFtQjtZQUFuQixtQkFBbUI7SUFDbkIsdUJBQTJCO1lBQTNCLDJCQUEyQjtJQUMzQixpQkFBaUI7SUFDakIsbUJBQW1CLEVBQUE7O0FBSXZCO0VBQ0Usb0JBQWE7RUFBYixhQUFhO0VBQ2IsNEJBQXNCO0VBQXRCLDZCQUFzQjtVQUF0QixzQkFBc0I7RUFDdEIsd0JBQXVCO1VBQXZCLHVCQUF1QjtFQUN2QixnQkFBZ0I7RUFDaEIsVUFBVSxFQUFBOztBQUxaO0lBUUksb0JBQWE7SUFBYixhQUFhO0lBQ2IsaUJBQWlCO0lBQ2pCLG1CQUFtQjtJQUNuQixtQkFBbUIsRUFBQTs7QUFJdkI7RUFDRSxvQkFBYTtFQUFiLGFBQWE7RUFDYix5QkFBbUI7VUFBbkIsbUJBQW1CO0VBQ25CLHlCQUE4QjtVQUE5Qiw4QkFBOEI7RUFDOUIsaUJBQWlCO0VBQ2pCLFVBQVU7RUFDVixjQUFjLEVBQUE7O0FBTmhCO0lBU0ksa0JBQWtCO0lBQ2xCLG9CQUFhO0lBQWIsYUFBYTtJQUNiLHlCQUFtQjtZQUFuQixtQkFBbUI7SUFDbkIsd0JBQXVCO1lBQXZCLHVCQUF1QjtJQUN2QixtQkFBYztZQUFkLGNBQWM7SUFDZCxnQkFBZ0I7SUFDaEIsVUFBVTtJQUNWLGNBQWMsRUFBQTs7QUFoQmxCO01BbUJNLGtCQUFrQjtNQUNsQixhQUFhO01BQ2IsU0FBUztNQUNULHdDQUFnQztjQUFoQyxnQ0FBZ0M7TUFDaEMsZUFBZTtNQUNmLG1CQUFtQixFQUFBOztBQUt6QjtFQUNFLFVBQVUsRUFBQTs7QUFEWjtJQUlJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLGlCQUFpQjtJQUNqQixtQkFBbUI7SUFDbkIsbUJBQW1CLEVBQUE7O0FBUHZCO0lBV0ksbUJBQW1CO0lBQ25CLFdBQVc7SUFDWCxnQkFBZ0IsRUFBQTs7QUFJcEI7RUFDRSxlQUFlLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9zZXR0aW5ncy9zZXR0aW5ncy5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIi5oZWFkIHtcbiAganVzdGlmeS1jb250ZW50OiBmbGV4LWVuZDtcbn1cblxuLnNldHRpbmdzLXRpdGxlIHtcbiAgZm9udC1zaXplOiAxLjdyZW07XG59XG5cbi50aGVtZS1zZWxlY3Rpb24ge1xuICBkaXNwbGF5OiBmbGV4O1xuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xuICBhbGlnbi1pdGVtczogZmxleC1zdGFydDtcbiAgbWFyZ2luOiAyLjRyZW0gMDtcbiAgd2lkdGg6IDUwJTtcblxuICAucmFkaW8tYmxvY2sge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGZsZXgtc3RhcnQ7XG4gICAgZm9udC1zaXplOiAxLjNyZW07XG4gICAgbGluZS1oZWlnaHQ6IDIuN3JlbTtcbiAgfVxufVxuXG4ubG9jay1zZWxlY3Rpb24ge1xuICBkaXNwbGF5OiBmbGV4O1xuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xuICBhbGlnbi1pdGVtczogZmxleC1zdGFydDtcbiAgbWFyZ2luOiAyLjRyZW0gMDtcbiAgd2lkdGg6IDUwJTtcblxuICAubG9jay1zZWxlY3Rpb24tdGl0bGUge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgZm9udC1zaXplOiAxLjVyZW07XG4gICAgbGluZS1oZWlnaHQ6IDIuN3JlbTtcbiAgICBtYXJnaW4tYm90dG9tOiAxcmVtO1xuICB9XG59XG5cbi5zY2FsZS1zZWxlY3Rpb24ge1xuICBkaXNwbGF5OiBmbGV4O1xuICBhbGlnbi1pdGVtczogY2VudGVyO1xuICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XG4gIHBhZGRpbmc6IDAgMCA0cmVtO1xuICB3aWR0aDogNTAlO1xuICBoZWlnaHQ6IDAuNXJlbTtcblxuICAuYnV0dG9uLWJsb2NrIHtcbiAgICBwb3NpdGlvbjogcmVsYXRpdmU7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xuICAgIGZsZXg6IDEgMCBhdXRvO1xuICAgIG1hcmdpbjogMCAwLjJyZW07XG4gICAgcGFkZGluZzogMDtcbiAgICBoZWlnaHQ6IDAuNXJlbTtcblxuICAgIC5sYWJlbCB7XG4gICAgICBwb3NpdGlvbjogYWJzb2x1dGU7XG4gICAgICBib3R0b206IC0xcmVtO1xuICAgICAgbGVmdDogNTAlO1xuICAgICAgdHJhbnNmb3JtOiB0cmFuc2xhdGUoLTUwJSwgMTAwJSk7XG4gICAgICBmb250LXNpemU6IDFyZW07XG4gICAgICB3aGl0ZS1zcGFjZTogbm93cmFwO1xuICAgIH1cbiAgfVxufVxuXG4ubWFzdGVyLXBhc3N3b3JkIHtcbiAgd2lkdGg6IDUwJTtcblxuICAubWFzdGVyLXBhc3N3b3JkLXRpdGxlIHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIGZvbnQtc2l6ZTogMS41cmVtO1xuICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XG4gICAgbWFyZ2luLWJvdHRvbTogMXJlbTtcbiAgfVxuXG4gIGJ1dHRvbiB7XG4gICAgbWFyZ2luOiAyLjVyZW0gYXV0bztcbiAgICB3aWR0aDogMTAwJTtcbiAgICBtYXgtd2lkdGg6IDE1cmVtO1xuICB9XG59XG5cbi5sYXN0LWJ1aWxkIHtcbiAgZm9udC1zaXplOiAxcmVtO1xufVxuIl19 */"

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
    function SettingsComponent(renderer, variablesService, backend, location, ngZone) {
        var _this = this;
        this.renderer = renderer;
        this.variablesService = variablesService;
        this.backend = backend;
        this.location = location;
        this.ngZone = ngZone;
        this.appLockOptions = [
            {
                id: 5,
                name: 'SETTINGS.APP_LOCK.TIME1'
            },
            {
                id: 15,
                name: 'SETTINGS.APP_LOCK.TIME2'
            },
            {
                id: 60,
                name: 'SETTINGS.APP_LOCK.TIME3'
            },
            {
                id: 0,
                name: 'SETTINGS.APP_LOCK.TIME4'
            }
        ];
        this.appScaleOptions = [
            {
                id: 7.5,
                name: '75% scale'
            },
            {
                id: 10,
                name: '100% scale'
            },
            {
                id: 12.5,
                name: '125% scale'
            },
            {
                id: 15,
                name: '150% scale'
            }
        ];
        this.appLogOptions = [
            {
                id: -1
            },
            {
                id: 0
            },
            {
                id: 1
            },
            {
                id: 2
            },
            {
                id: 3
            },
            {
                id: 4
            }
        ];
        this.currentBuild = '';
        this.theme = this.variablesService.settings.theme;
        this.scale = this.variablesService.settings.scale;
        this.changeForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_3__["FormGroup"]({
            password: new _angular_forms__WEBPACK_IMPORTED_MODULE_3__["FormControl"](''),
            new_password: new _angular_forms__WEBPACK_IMPORTED_MODULE_3__["FormControl"](''),
            new_confirmation: new _angular_forms__WEBPACK_IMPORTED_MODULE_3__["FormControl"]('')
        }, [function (g) {
                return g.get('new_password').value === g.get('new_confirmation').value ? null : { 'confirm_mismatch': true };
            }, function (g) {
                if (_this.variablesService.appPass) {
                    return g.get('password').value === _this.variablesService.appPass ? null : { 'pass_mismatch': true };
                }
                return null;
            }]);
    }
    SettingsComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.backend.getVersion(function (version) {
            _this.ngZone.run(function () {
                _this.currentBuild = version;
            });
        });
    };
    SettingsComponent.prototype.setTheme = function (theme) {
        this.renderer.removeClass(document.body, 'theme-' + this.theme);
        this.theme = theme;
        this.variablesService.settings.theme = this.theme;
        this.renderer.addClass(document.body, 'theme-' + this.theme);
        this.backend.storeAppData();
    };
    SettingsComponent.prototype.setScale = function (scale) {
        this.scale = scale;
        this.variablesService.settings.scale = this.scale;
        this.renderer.setStyle(document.documentElement, 'font-size', this.scale + 'px');
        this.backend.storeAppData();
    };
    SettingsComponent.prototype.onSubmitChangePass = function () {
        if (this.changeForm.valid) {
            this.variablesService.appPass = this.changeForm.get('new_password').value;
            if (this.variablesService.appPass) {
                this.backend.storeSecureAppData();
            }
            else {
                this.backend.dropSecureAppData();
            }
            this.changeForm.reset();
        }
    };
    SettingsComponent.prototype.onLockChange = function () {
        if (this.variablesService.appLogin) {
            this.variablesService.restartCountdown();
        }
        this.backend.storeAppData();
    };
    SettingsComponent.prototype.onLogChange = function () {
        this.backend.setLogLevel(this.variablesService.settings.appLog);
        this.backend.storeAppData();
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
        __metadata("design:paramtypes", [_angular_core__WEBPACK_IMPORTED_MODULE_0__["Renderer2"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _angular_common__WEBPACK_IMPORTED_MODULE_4__["Location"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
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

module.exports = "<div class=\"sidebar-accounts\">\n  <div class=\"sidebar-accounts-header\">\n    <h3>{{ 'SIDEBAR.TITLE' | translate }}</h3><button [routerLink]=\"['main']\">{{ 'SIDEBAR.ADD_NEW' | translate }}</button>\n  </div>\n  <div class=\"sidebar-accounts-list scrolled-content\">\n    <div class=\"sidebar-account\" *ngFor=\"let wallet of variablesService.wallets\" [class.active]=\"wallet?.wallet_id === walletActive\" [routerLink]=\"['/wallet/' + wallet.wallet_id + '/history']\">\n      <div class=\"sidebar-account-row account-title-balance\">\n        <span class=\"title\" tooltip=\"{{ wallet.name }}\" placement=\"top-left\" tooltipClass=\"table-tooltip account-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{wallet.name}}</span>\n        <span class=\"balance\">{{wallet.balance | intToMoney : '3' }} {{variablesService.defaultCurrency}}</span>\n      </div>\n      <div class=\"sidebar-account-row account-alias\">\n        <div class=\"name\">\n          <span tooltip=\"{{wallet.alias['name']}}\" placement=\"top-left\" tooltipClass=\"table-tooltip account-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{wallet.alias['name']}}</span>\n          <ng-container *ngIf=\"wallet.alias['comment'] && wallet.alias['comment'].length\">\n            <i class=\"icon comment\" tooltip=\"{{wallet.alias['comment']}}\" placement=\"top\" tooltipClass=\"table-tooltip account-tooltip\" [delay]=\"500\"></i>\n          </ng-container>\n        </div>\n        <span class=\"price\">$ {{wallet.getMoneyEquivalent(variablesService.moneyEquivalent) | intToMoney | number : '1.2-2'}}</span>\n      </div>\n      <div class=\"sidebar-account-row account-staking\" *ngIf=\"!(!wallet.loaded && variablesService.daemon_state === 2)\">\n        <span class=\"text\">{{ 'SIDEBAR.ACCOUNT.STAKING' | translate }}</span>\n        <app-staking-switch [wallet_id]=\"wallet.wallet_id\" [(staking)]=\"wallet.staking\"></app-staking-switch>\n      </div>\n      <div class=\"sidebar-account-row account-messages\" *ngIf=\"!(!wallet.loaded && variablesService.daemon_state === 2)\">\n        <span class=\"text\">{{ 'SIDEBAR.ACCOUNT.MESSAGES' | translate }}</span>\n        <span class=\"indicator\">{{wallet.new_contracts}}</span>\n      </div>\n      <div class=\"sidebar-account-row account-synchronization\" *ngIf=\"!wallet.loaded && variablesService.daemon_state === 2\">\n        <span class=\"status\">{{ 'SIDEBAR.ACCOUNT.SYNCING' | translate }}</span>\n        <div class=\"progress-bar-container\">\n          <div class=\"progress-bar\">\n            <div class=\"fill\" [style.width]=\"wallet.progress + '%'\"></div>\n          </div>\n          <div class=\"progress-percent\">{{ wallet.progress }}%</div>\n        </div>\n      </div>\n    </div>\n  </div>\n</div>\n<div class=\"sidebar-settings\">\n  <div class=\"wrap-button\" routerLinkActive=\"active\">\n    <button [routerLink]=\"['/settings']\">\n      <i class=\"icon settings\"></i>\n      <span>{{ 'SIDEBAR.SETTINGS' | translate }}</span>\n    </button>\n  </div>\n  <div class=\"wrap-button\">\n    <button (click)=\"logOut()\">\n      <i class=\"icon logout\"></i>\n      <span>{{ 'SIDEBAR.LOG_OUT' | translate }}</span>\n    </button>\n  </div>\n</div>\n<div class=\"sidebar-synchronization-status\">\n  <div class=\"status-container\">\n    <span class=\"offline\" *ngIf=\"variablesService.daemon_state === 0\">\n      {{ 'SIDEBAR.SYNCHRONIZATION.OFFLINE' | translate }}\n    </span>\n    <span class=\"syncing\" *ngIf=\"variablesService.daemon_state === 1\">\n      {{ 'SIDEBAR.SYNCHRONIZATION.SYNCING' | translate }}\n    </span>\n    <span class=\"online\" *ngIf=\"variablesService.daemon_state === 2\">\n      {{ 'SIDEBAR.SYNCHRONIZATION.ONLINE' | translate }}\n    </span>\n    <span class=\"loading\" *ngIf=\"variablesService.daemon_state === 3\">\n      {{ 'SIDEBAR.SYNCHRONIZATION.LOADING' | translate }}\n    </span>\n    <span class=\"offline\" *ngIf=\"variablesService.daemon_state === 4\">\n      {{ 'SIDEBAR.SYNCHRONIZATION.ERROR' | translate }}\n    </span>\n    <span class=\"online\" *ngIf=\"variablesService.daemon_state === 5\">\n      {{ 'SIDEBAR.SYNCHRONIZATION.COMPLETE' | translate }}\n    </span>\n    <div class=\"progress-bar-container\" *ngIf=\"variablesService.daemon_state === 1 || variablesService.daemon_state === 3\">\n      <div class=\"syncing\" *ngIf=\"variablesService.daemon_state === 1\">\n        <div class=\"progress-bar\">\n          <div class=\"fill\" [style.width]=\"variablesService.sync.progress_value + '%'\"></div>\n        </div>\n        <div class=\"progress-percent\">{{ variablesService.sync.progress_value_text }}%</div>\n      </div>\n      <div class=\"loading\" *ngIf=\"variablesService.daemon_state === 3\"></div>\n    </div>\n  </div>\n  <div class=\"update-container\" *ngIf=\"(variablesService.daemon_state === 0 || variablesService.daemon_state === 2) && [2, 3, 4].indexOf(variablesService.last_build_displaymode) !== -1\">\n    <ng-container *ngIf=\"variablesService.last_build_displaymode === 2\">\n      <div class=\"update-text standard\">\n        <span [style.cursor]=\"'pointer'\" (click)=\"getUpdate()\">{{ 'SIDEBAR.UPDATE.STANDARD' | translate }}</span>\n      </div>\n      <i class=\"icon update standard\" tooltip=\"{{ 'SIDEBAR.UPDATE.STANDARD_TOOLTIP' | translate }}\" placement=\"right-bottom\" tooltipClass=\"update-tooltip\" [delay]=\"500\"></i>\n    </ng-container>\n    <ng-container *ngIf=\"variablesService.last_build_displaymode === 3\">\n      <div class=\"update-text important\">\n        <span [style.cursor]=\"'pointer'\" (click)=\"getUpdate()\">{{ 'SIDEBAR.UPDATE.IMPORTANT' | translate }}</span>\n        <br>\n        <span style=\"font-size: 1rem\">{{ 'SIDEBAR.UPDATE.IMPORTANT_HINT' | translate }}</span>\n      </div>\n      <i class=\"icon update important\" tooltip=\"{{ 'SIDEBAR.UPDATE.IMPORTANT_TOOLTIP' | translate }}\" placement=\"right-bottom\" tooltipClass=\"update-tooltip important\" [delay]=\"500\"></i>\n    </ng-container>\n    <ng-container *ngIf=\"variablesService.last_build_displaymode === 4\">\n      <div class=\"update-text critical\">\n        <span [style.cursor]=\"'pointer'\" (click)=\"getUpdate()\">{{ 'SIDEBAR.UPDATE.CRITICAL' | translate }}</span>\n        <br>\n        <span style=\"font-size: 1rem\">{{ 'SIDEBAR.UPDATE.IMPORTANT_HINT' | translate }}</span>\n      </div>\n      <i class=\"icon update critical\" tooltip=\"{{ 'SIDEBAR.UPDATE.CRITICAL_TOOLTIP' | translate }}\" placement=\"right-bottom\" tooltipClass=\"update-tooltip critical\" [delay]=\"500\"></i>\n    </ng-container>\n  </div>\n  <div class=\"update-container\" *ngIf=\"variablesService.daemon_state === 2 && variablesService.net_time_delta_median !== 0\">\n    <div class=\"update-text time\">\n      <span>{{ 'SIDEBAR.UPDATE.TIME' | translate }}</span>\n    </div>\n    <i class=\"icon time\" tooltip=\"{{ 'SIDEBAR.UPDATE.TIME_TOOLTIP' | translate }}\" placement=\"right-bottom\" tooltipClass=\"update-tooltip important\" [delay]=\"500\"></i>\n  </div>\n</div>\n"

/***/ }),

/***/ "./src/app/sidebar/sidebar.component.scss":
/*!************************************************!*\
  !*** ./src/app/sidebar/sidebar.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  -webkit-box-pack: justify;\n          justify-content: space-between;\n  -webkit-box-flex: 0;\n          flex: 0 0 25rem;\n  padding: 0 3rem;\n  max-width: 25rem; }\n\n.sidebar-accounts {\n  position: relative;\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  -webkit-box-flex: 1;\n          flex: 1 1 auto; }\n\n.sidebar-accounts .sidebar-accounts-header {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: justify;\n            justify-content: space-between;\n    -webkit-box-flex: 0;\n            flex: 0 0 auto;\n    height: 8rem;\n    font-weight: 400; }\n\n.sidebar-accounts .sidebar-accounts-header h3 {\n      font-size: 1.7rem; }\n\n.sidebar-accounts .sidebar-accounts-header button {\n      background: transparent;\n      border: none;\n      outline: none; }\n\n.sidebar-accounts .sidebar-accounts-list {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-orient: vertical;\n    -webkit-box-direction: normal;\n            flex-direction: column;\n    -webkit-box-flex: 1;\n            flex: 1 1 auto;\n    margin: 0 -3rem;\n    overflow-y: overlay; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-orient: vertical;\n      -webkit-box-direction: normal;\n              flex-direction: column;\n      flex-shrink: 0;\n      cursor: pointer;\n      padding: 2rem 3rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row {\n        display: -webkit-box;\n        display: flex;\n        -webkit-box-align: center;\n                align-items: center;\n        -webkit-box-pack: justify;\n                justify-content: space-between; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-title-balance {\n          line-height: 2.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-title-balance .title {\n            font-size: 1.5rem;\n            text-overflow: ellipsis;\n            overflow: hidden;\n            white-space: nowrap; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-title-balance .balance {\n            font-size: 1.8rem;\n            font-weight: 600;\n            white-space: nowrap; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias {\n          font-size: 1.3rem;\n          line-height: 3.4rem;\n          margin-bottom: 0.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .name {\n            display: -webkit-box;\n            display: flex;\n            -webkit-box-align: center;\n                    align-items: center;\n            flex-shrink: 1;\n            line-height: 1.6rem;\n            padding-right: 1rem;\n            overflow: hidden; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .name span {\n              text-overflow: ellipsis;\n              overflow: hidden;\n              white-space: nowrap; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .price {\n            flex-shrink: 0; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .icon {\n            margin-left: 0.5rem;\n            width: 1.2rem;\n            height: 1.2rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .icon.comment {\n              -webkit-mask: url('alert.svg') no-repeat center;\n                      mask: url('alert.svg') no-repeat center; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-staking {\n          line-height: 2.9rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-staking .text {\n            font-size: 1.3rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-messages {\n          line-height: 2.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-messages .text {\n            font-size: 1.3rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-messages .indicator {\n            display: -webkit-box;\n            display: flex;\n            -webkit-box-align: center;\n                    align-items: center;\n            -webkit-box-pack: center;\n                    justify-content: center;\n            border-radius: 1rem;\n            font-size: 1rem;\n            min-width: 2.4rem;\n            height: 1.6rem;\n            padding: 0 0.5rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization {\n          -webkit-box-orient: vertical;\n          -webkit-box-direction: normal;\n                  flex-direction: column;\n          height: 5.6rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .status {\n            align-self: flex-start;\n            font-size: 1.3rem;\n            line-height: 2.6rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container {\n            display: -webkit-box;\n            display: flex;\n            margin: 0.4rem 0;\n            height: 0.7rem;\n            width: 100%; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container .progress-bar {\n              -webkit-box-flex: 1;\n                      flex: 1 0 auto; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container .progress-bar .fill {\n                height: 100%; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container .progress-percent {\n              -webkit-box-flex: 0;\n                      flex: 0 0 auto;\n              font-size: 1.3rem;\n              line-height: 0.7rem;\n              padding-left: 0.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account:focus {\n        outline: none; }\n\n.sidebar-accounts:after {\n    content: '';\n    position: absolute;\n    bottom: 0;\n    left: -3rem;\n    width: calc(100% + 6rem);\n    height: 5rem; }\n\n.sidebar-settings {\n  -webkit-box-flex: 0;\n          flex: 0 0 auto;\n  padding-bottom: 1rem; }\n\n.sidebar-settings .wrap-button {\n    margin: 0 -3rem; }\n\n.sidebar-settings .wrap-button button {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-align: center;\n              align-items: center;\n      background: transparent;\n      border: none;\n      font-weight: 400;\n      line-height: 3rem;\n      outline: none;\n      padding: 0 3rem;\n      width: 100%; }\n\n.sidebar-settings .wrap-button button .icon {\n        margin-right: 1.2rem;\n        width: 1.7rem;\n        height: 1.7rem; }\n\n.sidebar-settings .wrap-button button .icon.settings {\n          -webkit-mask: url('settings.svg') no-repeat center;\n                  mask: url('settings.svg') no-repeat center; }\n\n.sidebar-settings .wrap-button button .icon.logout {\n          -webkit-mask: url('logout.svg') no-repeat center;\n                  mask: url('logout.svg') no-repeat center; }\n\n.sidebar-synchronization-status {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  -webkit-box-pack: start;\n          justify-content: flex-start;\n  -webkit-box-flex: 0;\n          flex: 0 0 7rem;\n  font-size: 1.3rem; }\n\n.sidebar-synchronization-status .status-container {\n    position: relative;\n    -webkit-box-flex: 1;\n            flex-grow: 1;\n    text-align: left; }\n\n.sidebar-synchronization-status .status-container .offline, .sidebar-synchronization-status .status-container .online {\n      position: relative;\n      display: block;\n      line-height: 1.2rem;\n      padding-left: 2.2rem; }\n\n.sidebar-synchronization-status .status-container .offline:before, .sidebar-synchronization-status .status-container .online:before {\n        content: '';\n        position: absolute;\n        top: 0;\n        left: 0;\n        border-radius: 50%;\n        width: 1.2rem;\n        height: 1.2rem; }\n\n.sidebar-synchronization-status .status-container .syncing, .sidebar-synchronization-status .status-container .loading {\n      line-height: 4rem; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container {\n      position: absolute;\n      bottom: 0;\n      left: 0;\n      height: 0.7rem;\n      width: 100%; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .syncing {\n        display: -webkit-box;\n        display: flex; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .syncing .progress-bar {\n          -webkit-box-flex: 1;\n                  flex: 1 0 auto; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .syncing .progress-bar .fill {\n            height: 100%; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .syncing .progress-percent {\n          -webkit-box-flex: 0;\n                  flex: 0 0 auto;\n          font-size: 1.3rem;\n          line-height: 0.7rem;\n          padding-left: 0.7rem; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .loading {\n        -webkit-animation: move 5s linear infinite;\n                animation: move 5s linear infinite;\n        background-image: -webkit-gradient(linear, 0 0, 100% 100%, color-stop(0.125, rgba(0, 0, 0, 0.15)), color-stop(0.125, transparent), color-stop(0.25, transparent), color-stop(0.25, rgba(0, 0, 0, 0.1)), color-stop(0.375, rgba(0, 0, 0, 0.1)), color-stop(0.375, transparent), color-stop(0.5, transparent), color-stop(0.5, rgba(0, 0, 0, 0.15)), color-stop(0.625, rgba(0, 0, 0, 0.15)), color-stop(0.625, transparent), color-stop(0.75, transparent), color-stop(0.75, rgba(0, 0, 0, 0.1)), color-stop(0.875, rgba(0, 0, 0, 0.1)), color-stop(0.875, transparent), to(transparent)), -webkit-gradient(linear, 0 100%, 100% 0, color-stop(0.125, rgba(0, 0, 0, 0.3)), color-stop(0.125, transparent), color-stop(0.25, transparent), color-stop(0.25, rgba(0, 0, 0, 0.25)), color-stop(0.375, rgba(0, 0, 0, 0.25)), color-stop(0.375, transparent), color-stop(0.5, transparent), color-stop(0.5, rgba(0, 0, 0, 0.3)), color-stop(0.625, rgba(0, 0, 0, 0.3)), color-stop(0.625, transparent), color-stop(0.75, transparent), color-stop(0.75, rgba(0, 0, 0, 0.25)), color-stop(0.875, rgba(0, 0, 0, 0.25)), color-stop(0.875, transparent), to(transparent));\n        background-size: 7rem 7rem;\n        height: 100%; }\n\n.sidebar-synchronization-status .update-container {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-flex: 1;\n            flex-grow: 1;\n    margin-left: 1rem;\n    text-align: right; }\n\n.sidebar-synchronization-status .update-container .update-text {\n      -webkit-box-flex: 1;\n              flex: 1 1 auto;\n      font-size: 1.2rem;\n      line-height: 1.8rem;\n      text-align: left; }\n\n.sidebar-synchronization-status .update-container .update-text.time {\n        font-size: 1.1rem; }\n\n.sidebar-synchronization-status .update-container .icon {\n      -webkit-box-flex: 1;\n              flex: 1 0 auto;\n      margin: 0.3rem 0 0 0.6rem;\n      width: 1.2rem;\n      height: 1.2rem; }\n\n.sidebar-synchronization-status .update-container .icon.update {\n        -webkit-mask: url('update.svg') no-repeat center;\n                mask: url('update.svg') no-repeat center; }\n\n.sidebar-synchronization-status .update-container .icon.time {\n        -webkit-mask: url('time.svg') no-repeat center;\n                mask: url('time.svg') no-repeat center; }\n\n@-webkit-keyframes move {\n  0% {\n    background-position: 100% -7rem; }\n  100% {\n    background-position: 100% 7rem; } }\n\n@keyframes move {\n  0% {\n    background-position: 100% -7rem; }\n  100% {\n    background-position: 100% 7rem; } }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3NpZGViYXIvc2lkZWJhci5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLG9CQUFhO0VBQWIsYUFBYTtFQUNiLDRCQUFzQjtFQUF0Qiw2QkFBc0I7VUFBdEIsc0JBQXNCO0VBQ3RCLHlCQUE4QjtVQUE5Qiw4QkFBOEI7RUFDOUIsbUJBQWU7VUFBZixlQUFlO0VBQ2YsZUFBZTtFQUNmLGdCQUFnQixFQUFBOztBQUdsQjtFQUNFLGtCQUFrQjtFQUNsQixvQkFBYTtFQUFiLGFBQWE7RUFDYiw0QkFBc0I7RUFBdEIsNkJBQXNCO1VBQXRCLHNCQUFzQjtFQUN0QixtQkFBYztVQUFkLGNBQWMsRUFBQTs7QUFKaEI7SUFPSSxvQkFBYTtJQUFiLGFBQWE7SUFDYix5QkFBbUI7WUFBbkIsbUJBQW1CO0lBQ25CLHlCQUE4QjtZQUE5Qiw4QkFBOEI7SUFDOUIsbUJBQWM7WUFBZCxjQUFjO0lBQ2QsWUFBWTtJQUNaLGdCQUFnQixFQUFBOztBQVpwQjtNQWVNLGlCQUFpQixFQUFBOztBQWZ2QjtNQW1CTSx1QkFBdUI7TUFDdkIsWUFBWTtNQUNaLGFBQWEsRUFBQTs7QUFyQm5CO0lBMEJJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLDRCQUFzQjtJQUF0Qiw2QkFBc0I7WUFBdEIsc0JBQXNCO0lBQ3RCLG1CQUFjO1lBQWQsY0FBYztJQUNkLGVBQWU7SUFDZixtQkFBbUIsRUFBQTs7QUE5QnZCO01BaUNNLG9CQUFhO01BQWIsYUFBYTtNQUNiLDRCQUFzQjtNQUF0Qiw2QkFBc0I7Y0FBdEIsc0JBQXNCO01BQ3RCLGNBQWM7TUFDZCxlQUFlO01BQ2Ysa0JBQWtCLEVBQUE7O0FBckN4QjtRQXdDUSxvQkFBYTtRQUFiLGFBQWE7UUFDYix5QkFBbUI7Z0JBQW5CLG1CQUFtQjtRQUNuQix5QkFBOEI7Z0JBQTlCLDhCQUE4QixFQUFBOztBQTFDdEM7VUE2Q1UsbUJBQW1CLEVBQUE7O0FBN0M3QjtZQWdEWSxpQkFBaUI7WUFDakIsdUJBQXVCO1lBQ3ZCLGdCQUFnQjtZQUNoQixtQkFBbUIsRUFBQTs7QUFuRC9CO1lBdURZLGlCQUFpQjtZQUNqQixnQkFBZ0I7WUFDaEIsbUJBQW1CLEVBQUE7O0FBekQvQjtVQThEVSxpQkFBaUI7VUFDakIsbUJBQW1CO1VBQ25CLHFCQUFxQixFQUFBOztBQWhFL0I7WUFtRVksb0JBQWE7WUFBYixhQUFhO1lBQ2IseUJBQW1CO29CQUFuQixtQkFBbUI7WUFDbkIsY0FBYztZQUNkLG1CQUFtQjtZQUNuQixtQkFBbUI7WUFDbkIsZ0JBQWdCLEVBQUE7O0FBeEU1QjtjQTJFYyx1QkFBdUI7Y0FDdkIsZ0JBQWdCO2NBQ2hCLG1CQUFtQixFQUFBOztBQTdFakM7WUFrRlksY0FBYyxFQUFBOztBQWxGMUI7WUFzRlksbUJBQW1CO1lBQ25CLGFBQWE7WUFDYixjQUFjLEVBQUE7O0FBeEYxQjtjQTJGYywrQ0FBd0Q7c0JBQXhELHVDQUF3RCxFQUFBOztBQTNGdEU7VUFpR1UsbUJBQW1CLEVBQUE7O0FBakc3QjtZQW9HWSxpQkFBaUIsRUFBQTs7QUFwRzdCO1VBeUdVLG1CQUFtQixFQUFBOztBQXpHN0I7WUE0R1ksaUJBQWlCLEVBQUE7O0FBNUc3QjtZQWdIWSxvQkFBYTtZQUFiLGFBQWE7WUFDYix5QkFBbUI7b0JBQW5CLG1CQUFtQjtZQUNuQix3QkFBdUI7b0JBQXZCLHVCQUF1QjtZQUN2QixtQkFBbUI7WUFDbkIsZUFBZTtZQUNmLGlCQUFpQjtZQUNqQixjQUFjO1lBQ2QsaUJBQWlCLEVBQUE7O0FBdkg3QjtVQTRIVSw0QkFBc0I7VUFBdEIsNkJBQXNCO2tCQUF0QixzQkFBc0I7VUFDdEIsY0FBYyxFQUFBOztBQTdIeEI7WUFnSVksc0JBQXNCO1lBQ3RCLGlCQUFpQjtZQUNqQixtQkFBbUIsRUFBQTs7QUFsSS9CO1lBc0lZLG9CQUFhO1lBQWIsYUFBYTtZQUNiLGdCQUFnQjtZQUNoQixjQUFjO1lBQ2QsV0FBVyxFQUFBOztBQXpJdkI7Y0E0SWMsbUJBQWM7c0JBQWQsY0FBYyxFQUFBOztBQTVJNUI7Z0JBK0lnQixZQUFZLEVBQUE7O0FBL0k1QjtjQW9KYyxtQkFBYztzQkFBZCxjQUFjO2NBQ2QsaUJBQWlCO2NBQ2pCLG1CQUFtQjtjQUNuQixvQkFBb0IsRUFBQTs7QUF2SmxDO1FBOEpRLGFBQWEsRUFBQTs7QUE5SnJCO0lBb0tJLFdBQVc7SUFDWCxrQkFBa0I7SUFDbEIsU0FBUztJQUNULFdBQVc7SUFDWCx3QkFBd0I7SUFDeEIsWUFBWSxFQUFBOztBQUloQjtFQUNFLG1CQUFjO1VBQWQsY0FBYztFQUNkLG9CQUFvQixFQUFBOztBQUZ0QjtJQUtJLGVBQWUsRUFBQTs7QUFMbkI7TUFRTSxvQkFBYTtNQUFiLGFBQWE7TUFDYix5QkFBbUI7Y0FBbkIsbUJBQW1CO01BQ25CLHVCQUF1QjtNQUN2QixZQUFZO01BQ1osZ0JBQWdCO01BQ2hCLGlCQUFpQjtNQUNqQixhQUFhO01BQ2IsZUFBZTtNQUNmLFdBQVcsRUFBQTs7QUFoQmpCO1FBbUJRLG9CQUFvQjtRQUNwQixhQUFhO1FBQ2IsY0FBYyxFQUFBOztBQXJCdEI7VUF3QlUsa0RBQTJEO2tCQUEzRCwwQ0FBMkQsRUFBQTs7QUF4QnJFO1VBNEJVLGdEQUF5RDtrQkFBekQsd0NBQXlELEVBQUE7O0FBT25FO0VBQ0Usb0JBQWE7RUFBYixhQUFhO0VBQ2IseUJBQW1CO1VBQW5CLG1CQUFtQjtFQUNuQix1QkFBMkI7VUFBM0IsMkJBQTJCO0VBQzNCLG1CQUFjO1VBQWQsY0FBYztFQUNkLGlCQUFpQixFQUFBOztBQUxuQjtJQVFJLGtCQUFrQjtJQUNsQixtQkFBWTtZQUFaLFlBQVk7SUFDWixnQkFBZ0IsRUFBQTs7QUFWcEI7TUFhTSxrQkFBa0I7TUFDbEIsY0FBYztNQUNkLG1CQUFtQjtNQUNuQixvQkFBb0IsRUFBQTs7QUFoQjFCO1FBbUJRLFdBQVc7UUFDWCxrQkFBa0I7UUFDbEIsTUFBTTtRQUNOLE9BQU87UUFDUCxrQkFBa0I7UUFDbEIsYUFBYTtRQUNiLGNBQWMsRUFBQTs7QUF6QnRCO01BOEJNLGlCQUFpQixFQUFBOztBQTlCdkI7TUFrQ00sa0JBQWtCO01BQ2xCLFNBQVM7TUFDVCxPQUFPO01BQ1AsY0FBYztNQUNkLFdBQVcsRUFBQTs7QUF0Q2pCO1FBeUNRLG9CQUFhO1FBQWIsYUFBYSxFQUFBOztBQXpDckI7VUE0Q1UsbUJBQWM7a0JBQWQsY0FBYyxFQUFBOztBQTVDeEI7WUErQ1ksWUFBWSxFQUFBOztBQS9DeEI7VUFvRFUsbUJBQWM7a0JBQWQsY0FBYztVQUNkLGlCQUFpQjtVQUNqQixtQkFBbUI7VUFDbkIsb0JBQW9CLEVBQUE7O0FBdkQ5QjtRQTREUSwwQ0FBa0M7Z0JBQWxDLGtDQUFrQztRQUNsQywrbENBc0JHO1FBQ0gsMEJBQTBCO1FBQzFCLFlBQVksRUFBQTs7QUFyRnBCO0lBMkZJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLG1CQUFZO1lBQVosWUFBWTtJQUNaLGlCQUFpQjtJQUNqQixpQkFBaUIsRUFBQTs7QUE5RnJCO01BaUdNLG1CQUFjO2NBQWQsY0FBYztNQUNkLGlCQUFpQjtNQUNqQixtQkFBbUI7TUFDbkIsZ0JBQWdCLEVBQUE7O0FBcEd0QjtRQXVHUSxpQkFBaUIsRUFBQTs7QUF2R3pCO01BNEdNLG1CQUFjO2NBQWQsY0FBYztNQUNkLHlCQUF5QjtNQUN6QixhQUFhO01BQ2IsY0FBYyxFQUFBOztBQS9HcEI7UUFrSFEsZ0RBQXlEO2dCQUF6RCx3Q0FBeUQsRUFBQTs7QUFsSGpFO1FBc0hRLDhDQUF1RDtnQkFBdkQsc0NBQXVELEVBQUE7O0FBTS9EO0VBQ0U7SUFDRSwrQkFBK0IsRUFBQTtFQUVqQztJQUNFLDhCQUE4QixFQUFBLEVBQUE7O0FBTGxDO0VBQ0U7SUFDRSwrQkFBK0IsRUFBQTtFQUVqQztJQUNFLDhCQUE4QixFQUFBLEVBQUEiLCJmaWxlIjoic3JjL2FwcC9zaWRlYmFyL3NpZGViYXIuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XG4gIGRpc3BsYXk6IGZsZXg7XG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XG4gIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcbiAgZmxleDogMCAwIDI1cmVtO1xuICBwYWRkaW5nOiAwIDNyZW07XG4gIG1heC13aWR0aDogMjVyZW07XG59XG5cbi5zaWRlYmFyLWFjY291bnRzIHtcbiAgcG9zaXRpb246IHJlbGF0aXZlO1xuICBkaXNwbGF5OiBmbGV4O1xuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xuICBmbGV4OiAxIDEgYXV0bztcblxuICAuc2lkZWJhci1hY2NvdW50cy1oZWFkZXIge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XG4gICAgZmxleDogMCAwIGF1dG87XG4gICAgaGVpZ2h0OiA4cmVtO1xuICAgIGZvbnQtd2VpZ2h0OiA0MDA7XG5cbiAgICBoMyB7XG4gICAgICBmb250LXNpemU6IDEuN3JlbTtcbiAgICB9XG5cbiAgICBidXR0b24ge1xuICAgICAgYmFja2dyb3VuZDogdHJhbnNwYXJlbnQ7XG4gICAgICBib3JkZXI6IG5vbmU7XG4gICAgICBvdXRsaW5lOiBub25lO1xuICAgIH1cbiAgfVxuXG4gIC5zaWRlYmFyLWFjY291bnRzLWxpc3Qge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcbiAgICBmbGV4OiAxIDEgYXV0bztcbiAgICBtYXJnaW46IDAgLTNyZW07XG4gICAgb3ZlcmZsb3cteTogb3ZlcmxheTtcblxuICAgIC5zaWRlYmFyLWFjY291bnQge1xuICAgICAgZGlzcGxheTogZmxleDtcbiAgICAgIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XG4gICAgICBmbGV4LXNocmluazogMDtcbiAgICAgIGN1cnNvcjogcG9pbnRlcjtcbiAgICAgIHBhZGRpbmc6IDJyZW0gM3JlbTtcblxuICAgICAgLnNpZGViYXItYWNjb3VudC1yb3cge1xuICAgICAgICBkaXNwbGF5OiBmbGV4O1xuICAgICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgICAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XG5cbiAgICAgICAgJi5hY2NvdW50LXRpdGxlLWJhbGFuY2Uge1xuICAgICAgICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XG5cbiAgICAgICAgICAudGl0bGUge1xuICAgICAgICAgICAgZm9udC1zaXplOiAxLjVyZW07XG4gICAgICAgICAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcbiAgICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XG4gICAgICAgICAgICB3aGl0ZS1zcGFjZTogbm93cmFwO1xuICAgICAgICAgIH1cblxuICAgICAgICAgIC5iYWxhbmNlIHtcbiAgICAgICAgICAgIGZvbnQtc2l6ZTogMS44cmVtO1xuICAgICAgICAgICAgZm9udC13ZWlnaHQ6IDYwMDtcbiAgICAgICAgICAgIHdoaXRlLXNwYWNlOiBub3dyYXA7XG4gICAgICAgICAgfVxuICAgICAgICB9XG5cbiAgICAgICAgJi5hY2NvdW50LWFsaWFzIHtcbiAgICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICAgICAgICBsaW5lLWhlaWdodDogMy40cmVtO1xuICAgICAgICAgIG1hcmdpbi1ib3R0b206IDAuN3JlbTtcblxuICAgICAgICAgIC5uYW1lIHtcbiAgICAgICAgICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgICAgICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgICAgICAgICAgZmxleC1zaHJpbms6IDE7XG4gICAgICAgICAgICBsaW5lLWhlaWdodDogMS42cmVtO1xuICAgICAgICAgICAgcGFkZGluZy1yaWdodDogMXJlbTtcbiAgICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XG5cbiAgICAgICAgICAgIHNwYW4ge1xuICAgICAgICAgICAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcbiAgICAgICAgICAgICAgb3ZlcmZsb3c6IGhpZGRlbjtcbiAgICAgICAgICAgICAgd2hpdGUtc3BhY2U6IG5vd3JhcDtcbiAgICAgICAgICAgIH1cbiAgICAgICAgICB9XG5cbiAgICAgICAgICAucHJpY2Uge1xuICAgICAgICAgICAgZmxleC1zaHJpbms6IDA7XG4gICAgICAgICAgfVxuXG4gICAgICAgICAgLmljb24ge1xuICAgICAgICAgICAgbWFyZ2luLWxlZnQ6IDAuNXJlbTtcbiAgICAgICAgICAgIHdpZHRoOiAxLjJyZW07XG4gICAgICAgICAgICBoZWlnaHQ6IDEuMnJlbTtcblxuICAgICAgICAgICAgJi5jb21tZW50IHtcbiAgICAgICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9hbGVydC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICAgICAgICB9XG4gICAgICAgICAgfVxuICAgICAgICB9XG5cbiAgICAgICAgJi5hY2NvdW50LXN0YWtpbmcge1xuICAgICAgICAgIGxpbmUtaGVpZ2h0OiAyLjlyZW07XG5cbiAgICAgICAgICAudGV4dCB7XG4gICAgICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICAgICAgICB9XG4gICAgICAgIH1cblxuICAgICAgICAmLmFjY291bnQtbWVzc2FnZXMge1xuICAgICAgICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XG5cbiAgICAgICAgICAudGV4dCB7XG4gICAgICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICAgICAgICB9XG5cbiAgICAgICAgICAuaW5kaWNhdG9yIHtcbiAgICAgICAgICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgICAgICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgICAgICAgICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XG4gICAgICAgICAgICBib3JkZXItcmFkaXVzOiAxcmVtO1xuICAgICAgICAgICAgZm9udC1zaXplOiAxcmVtO1xuICAgICAgICAgICAgbWluLXdpZHRoOiAyLjRyZW07XG4gICAgICAgICAgICBoZWlnaHQ6IDEuNnJlbTtcbiAgICAgICAgICAgIHBhZGRpbmc6IDAgMC41cmVtO1xuICAgICAgICAgIH1cbiAgICAgICAgfVxuXG4gICAgICAgICYuYWNjb3VudC1zeW5jaHJvbml6YXRpb24ge1xuICAgICAgICAgIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XG4gICAgICAgICAgaGVpZ2h0OiA1LjZyZW07XG5cbiAgICAgICAgICAuc3RhdHVzIHtcbiAgICAgICAgICAgIGFsaWduLXNlbGY6IGZsZXgtc3RhcnQ7XG4gICAgICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICAgICAgICAgIGxpbmUtaGVpZ2h0OiAyLjZyZW07XG4gICAgICAgICAgfVxuXG4gICAgICAgICAgLnByb2dyZXNzLWJhci1jb250YWluZXIge1xuICAgICAgICAgICAgZGlzcGxheTogZmxleDtcbiAgICAgICAgICAgIG1hcmdpbjogMC40cmVtIDA7XG4gICAgICAgICAgICBoZWlnaHQ6IDAuN3JlbTtcbiAgICAgICAgICAgIHdpZHRoOiAxMDAlO1xuXG4gICAgICAgICAgICAucHJvZ3Jlc3MtYmFyIHtcbiAgICAgICAgICAgICAgZmxleDogMSAwIGF1dG87XG5cbiAgICAgICAgICAgICAgLmZpbGwge1xuICAgICAgICAgICAgICAgIGhlaWdodDogMTAwJTtcbiAgICAgICAgICAgICAgfVxuICAgICAgICAgICAgfVxuXG4gICAgICAgICAgICAucHJvZ3Jlc3MtcGVyY2VudCB7XG4gICAgICAgICAgICAgIGZsZXg6IDAgMCBhdXRvO1xuICAgICAgICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICAgICAgICAgICAgbGluZS1oZWlnaHQ6IDAuN3JlbTtcbiAgICAgICAgICAgICAgcGFkZGluZy1sZWZ0OiAwLjdyZW07XG4gICAgICAgICAgICB9XG4gICAgICAgICAgfVxuICAgICAgICB9XG4gICAgICB9XG5cbiAgICAgICY6Zm9jdXMge1xuICAgICAgICBvdXRsaW5lOiBub25lO1xuICAgICAgfVxuICAgIH1cbiAgfVxuXG4gICY6YWZ0ZXIge1xuICAgIGNvbnRlbnQ6ICcnO1xuICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcbiAgICBib3R0b206IDA7XG4gICAgbGVmdDogLTNyZW07XG4gICAgd2lkdGg6IGNhbGMoMTAwJSArIDZyZW0pO1xuICAgIGhlaWdodDogNXJlbTtcbiAgfVxufVxuXG4uc2lkZWJhci1zZXR0aW5ncyB7XG4gIGZsZXg6IDAgMCBhdXRvO1xuICBwYWRkaW5nLWJvdHRvbTogMXJlbTtcblxuICAud3JhcC1idXR0b24ge1xuICAgIG1hcmdpbjogMCAtM3JlbTtcblxuICAgIGJ1dHRvbiB7XG4gICAgICBkaXNwbGF5OiBmbGV4O1xuICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICAgIGJhY2tncm91bmQ6IHRyYW5zcGFyZW50O1xuICAgICAgYm9yZGVyOiBub25lO1xuICAgICAgZm9udC13ZWlnaHQ6IDQwMDtcbiAgICAgIGxpbmUtaGVpZ2h0OiAzcmVtO1xuICAgICAgb3V0bGluZTogbm9uZTtcbiAgICAgIHBhZGRpbmc6IDAgM3JlbTtcbiAgICAgIHdpZHRoOiAxMDAlO1xuXG4gICAgICAuaWNvbiB7XG4gICAgICAgIG1hcmdpbi1yaWdodDogMS4ycmVtO1xuICAgICAgICB3aWR0aDogMS43cmVtO1xuICAgICAgICBoZWlnaHQ6IDEuN3JlbTtcblxuICAgICAgICAmLnNldHRpbmdzIHtcbiAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3NldHRpbmdzLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgICAgfVxuXG4gICAgICAgICYubG9nb3V0IHtcbiAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2xvZ291dC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICAgIH1cbiAgICAgIH1cbiAgICB9XG4gIH1cbn1cblxuLnNpZGViYXItc3luY2hyb25pemF0aW9uLXN0YXR1cyB7XG4gIGRpc3BsYXk6IGZsZXg7XG4gIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG4gIGp1c3RpZnktY29udGVudDogZmxleC1zdGFydDtcbiAgZmxleDogMCAwIDdyZW07XG4gIGZvbnQtc2l6ZTogMS4zcmVtO1xuXG4gIC5zdGF0dXMtY29udGFpbmVyIHtcbiAgICBwb3NpdGlvbjogcmVsYXRpdmU7XG4gICAgZmxleC1ncm93OiAxO1xuICAgIHRleHQtYWxpZ246IGxlZnQ7XG5cbiAgICAub2ZmbGluZSwgLm9ubGluZSB7XG4gICAgICBwb3NpdGlvbjogcmVsYXRpdmU7XG4gICAgICBkaXNwbGF5OiBibG9jaztcbiAgICAgIGxpbmUtaGVpZ2h0OiAxLjJyZW07XG4gICAgICBwYWRkaW5nLWxlZnQ6IDIuMnJlbTtcblxuICAgICAgJjpiZWZvcmUge1xuICAgICAgICBjb250ZW50OiAnJztcbiAgICAgICAgcG9zaXRpb246IGFic29sdXRlO1xuICAgICAgICB0b3A6IDA7XG4gICAgICAgIGxlZnQ6IDA7XG4gICAgICAgIGJvcmRlci1yYWRpdXM6IDUwJTtcbiAgICAgICAgd2lkdGg6IDEuMnJlbTtcbiAgICAgICAgaGVpZ2h0OiAxLjJyZW07XG4gICAgICB9XG4gICAgfVxuXG4gICAgLnN5bmNpbmcsIC5sb2FkaW5nIHtcbiAgICAgIGxpbmUtaGVpZ2h0OiA0cmVtO1xuICAgIH1cblxuICAgIC5wcm9ncmVzcy1iYXItY29udGFpbmVyIHtcbiAgICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcbiAgICAgIGJvdHRvbTogMDtcbiAgICAgIGxlZnQ6IDA7XG4gICAgICBoZWlnaHQ6IDAuN3JlbTtcbiAgICAgIHdpZHRoOiAxMDAlO1xuXG4gICAgICAuc3luY2luZyB7XG4gICAgICAgIGRpc3BsYXk6IGZsZXg7XG5cbiAgICAgICAgLnByb2dyZXNzLWJhciB7XG4gICAgICAgICAgZmxleDogMSAwIGF1dG87XG5cbiAgICAgICAgICAuZmlsbCB7XG4gICAgICAgICAgICBoZWlnaHQ6IDEwMCU7XG4gICAgICAgICAgfVxuICAgICAgICB9XG5cbiAgICAgICAgLnByb2dyZXNzLXBlcmNlbnQge1xuICAgICAgICAgIGZsZXg6IDAgMCBhdXRvO1xuICAgICAgICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xuICAgICAgICAgIGxpbmUtaGVpZ2h0OiAwLjdyZW07XG4gICAgICAgICAgcGFkZGluZy1sZWZ0OiAwLjdyZW07XG4gICAgICAgIH1cbiAgICAgIH1cblxuICAgICAgLmxvYWRpbmcge1xuICAgICAgICBhbmltYXRpb246IG1vdmUgNXMgbGluZWFyIGluZmluaXRlO1xuICAgICAgICBiYWNrZ3JvdW5kLWltYWdlOlxuICAgICAgICAgIC13ZWJraXQtZ3JhZGllbnQoXG4gICAgICAgICAgICAgIGxpbmVhciwgMCAwLCAxMDAlIDEwMCUsXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjEyNSwgcmdiYSgwLCAwLCAwLCAuMTUpKSwgY29sb3Itc3RvcCguMTI1LCB0cmFuc3BhcmVudCksXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjI1MCwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKC4yNTAsIHJnYmEoMCwgMCwgMCwgLjEwKSksXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjM3NSwgcmdiYSgwLCAwLCAwLCAuMTApKSwgY29sb3Itc3RvcCguMzc1LCB0cmFuc3BhcmVudCksXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjUwMCwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKC41MDAsIHJnYmEoMCwgMCwgMCwgLjE1KSksXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjYyNSwgcmdiYSgwLCAwLCAwLCAuMTUpKSwgY29sb3Itc3RvcCguNjI1LCB0cmFuc3BhcmVudCksXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjc1MCwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKC43NTAsIHJnYmEoMCwgMCwgMCwgLjEwKSksXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjg3NSwgcmdiYSgwLCAwLCAwLCAuMTApKSwgY29sb3Itc3RvcCguODc1LCB0cmFuc3BhcmVudCksXG4gICAgICAgICAgICAgIHRvKHRyYW5zcGFyZW50KVxuICAgICAgICAgICksXG4gICAgICAgICAgLXdlYmtpdC1ncmFkaWVudChcbiAgICAgICAgICAgICAgbGluZWFyLCAwIDEwMCUsIDEwMCUgMCxcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguMTI1LCByZ2JhKDAsIDAsIDAsIC4zMCkpLCBjb2xvci1zdG9wKC4xMjUsIHRyYW5zcGFyZW50KSxcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguMjUwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjI1MCwgcmdiYSgwLCAwLCAwLCAuMjUpKSxcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguMzc1LCByZ2JhKDAsIDAsIDAsIC4yNSkpLCBjb2xvci1zdG9wKC4zNzUsIHRyYW5zcGFyZW50KSxcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguNTAwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjUwMCwgcmdiYSgwLCAwLCAwLCAuMzApKSxcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguNjI1LCByZ2JhKDAsIDAsIDAsIC4zMCkpLCBjb2xvci1zdG9wKC42MjUsIHRyYW5zcGFyZW50KSxcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguNzUwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjc1MCwgcmdiYSgwLCAwLCAwLCAuMjUpKSxcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguODc1LCByZ2JhKDAsIDAsIDAsIC4yNSkpLCBjb2xvci1zdG9wKC44NzUsIHRyYW5zcGFyZW50KSxcbiAgICAgICAgICAgICAgdG8odHJhbnNwYXJlbnQpXG4gICAgICAgICAgKTtcbiAgICAgICAgYmFja2dyb3VuZC1zaXplOiA3cmVtIDdyZW07XG4gICAgICAgIGhlaWdodDogMTAwJTtcbiAgICAgIH1cbiAgICB9XG4gIH1cblxuICAudXBkYXRlLWNvbnRhaW5lciB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBmbGV4LWdyb3c6IDE7XG4gICAgbWFyZ2luLWxlZnQ6IDFyZW07XG4gICAgdGV4dC1hbGlnbjogcmlnaHQ7XG5cbiAgICAudXBkYXRlLXRleHQge1xuICAgICAgZmxleDogMSAxIGF1dG87XG4gICAgICBmb250LXNpemU6IDEuMnJlbTtcbiAgICAgIGxpbmUtaGVpZ2h0OiAxLjhyZW07XG4gICAgICB0ZXh0LWFsaWduOiBsZWZ0O1xuXG4gICAgICAmLnRpbWUge1xuICAgICAgICBmb250LXNpemU6IDEuMXJlbTtcbiAgICAgIH1cbiAgICB9XG5cbiAgICAuaWNvbiB7XG4gICAgICBmbGV4OiAxIDAgYXV0bztcbiAgICAgIG1hcmdpbjogMC4zcmVtIDAgMCAwLjZyZW07XG4gICAgICB3aWR0aDogMS4ycmVtO1xuICAgICAgaGVpZ2h0OiAxLjJyZW07XG5cbiAgICAgICYudXBkYXRlIHtcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy91cGRhdGUuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xuICAgICAgfVxuXG4gICAgICAmLnRpbWUge1xuICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3RpbWUuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xuICAgICAgfVxuICAgIH1cbiAgfVxufVxuXG5Aa2V5ZnJhbWVzIG1vdmUge1xuICAwJSB7XG4gICAgYmFja2dyb3VuZC1wb3NpdGlvbjogMTAwJSAtN3JlbTtcbiAgfVxuICAxMDAlIHtcbiAgICBiYWNrZ3JvdW5kLXBvc2l0aW9uOiAxMDAlIDdyZW07XG4gIH1cbn1cbiJdfQ== */"

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




var SidebarComponent = /** @class */ (function () {
    function SidebarComponent(route, router, variablesService, backend, ngZone) {
        this.route = route;
        this.router = router;
        this.variablesService = variablesService;
        this.backend = backend;
        this.ngZone = ngZone;
    }
    SidebarComponent.prototype.ngOnInit = function () {
        var _this = this;
        if (this.router.url.indexOf('/wallet/') !== -1) {
            var localPathArr = this.router.url.split('/');
            if (localPathArr.length >= 3) {
                this.walletActive = parseInt(localPathArr[2], 10);
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
    SidebarComponent.prototype.getUpdate = function () {
        this.backend.openUrlInBrowser('zano.org/downloads.html');
    };
    SidebarComponent.prototype.logOut = function () {
        var _this = this;
        this.variablesService.stopCountdown();
        this.variablesService.appLogin = false;
        this.variablesService.appPass = '';
        this.ngZone.run(function () {
            _this.router.navigate(['/login'], { queryParams: { type: 'auth' } });
        });
    };
    SidebarComponent.prototype.ngOnDestroy = function () {
        this.walletSubRouting.unsubscribe();
    };
    SidebarComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-sidebar',
            template: __webpack_require__(/*! ./sidebar.component.html */ "./src/app/sidebar/sidebar.component.html"),
            styles: [__webpack_require__(/*! ./sidebar.component.scss */ "./src/app/sidebar/sidebar.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_1__["ActivatedRoute"],
            _angular_router__WEBPACK_IMPORTED_MODULE_1__["Router"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
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

module.exports = "<div class=\"chart-header\">\n  <div class=\"general\">\n    <div>\n      <span class=\"label\">{{ 'STAKING.TITLE' | translate }}</span>\n      <span class=\"value\">\n        <app-staking-switch [wallet_id]=\"variablesService.currentWallet.wallet_id\" [(staking)]=\"variablesService.currentWallet.staking\"></app-staking-switch>\n      </span>\n    </div>\n    <div>\n      <span class=\"label\">{{ 'STAKING.TITLE_PENDING' | translate }}</span>\n      <span class=\"value\">{{pending.total | intToMoney}} {{variablesService.defaultCurrency}}</span>\n    </div>\n    <div>\n      <span class=\"label\">{{ 'STAKING.TITLE_TOTAL' | translate }}</span>\n      <span class=\"value\">{{total | intToMoney}} {{variablesService.defaultCurrency}}</span>\n    </div>\n  </div>\n  <div class=\"selected\" *ngIf=\"selectedDate && selectedDate.date\">\n    <span>{{selectedDate.date | date : 'MMM. EEEE, dd, yyyy'}}</span>\n    <span>{{selectedDate.amount}} {{variablesService.defaultCurrency}}</span>\n  </div>\n</div>\n\n<div class=\"chart\">\n  <div [chart]=\"chart\"></div>\n</div>\n\n<div class=\"chart-options\">\n  <div class=\"title\">\n    {{ 'STAKING.TITLE_PERIOD' | translate }}\n  </div>\n  <div class=\"options\">\n    <ng-container *ngFor=\"let period of periods\">\n      <button type=\"button\" [class.active]=\"period.active\" (click)=\"changePeriod(period)\">{{period.title}}</button>\n    </ng-container>\n  </div>\n\n  <div class=\"title\">\n    {{ 'STAKING.TITLE_GROUP' | translate }}\n  </div>\n  <div class=\"options\">\n    <ng-container *ngFor=\"let group of groups\">\n      <button type=\"button\" [class.active]=\"group.active\" (click)=\"changeGroup(group)\">{{group.title}}</button>\n    </ng-container>\n  </div>\n</div>\n"

/***/ }),

/***/ "./src/app/staking/staking.component.scss":
/*!************************************************!*\
  !*** ./src/app/staking/staking.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  width: 100%; }\n\n.chart-header {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-flex: 0;\n          flex: 0 0 auto; }\n\n.chart-header .general {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-orient: vertical;\n    -webkit-box-direction: normal;\n            flex-direction: column;\n    -webkit-box-align: start;\n            align-items: flex-start;\n    -webkit-box-pack: center;\n            justify-content: center;\n    -webkit-box-flex: 1;\n            flex-grow: 1;\n    font-size: 1.3rem;\n    margin: -0.5rem 0; }\n\n.chart-header .general > div {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-align: center;\n              align-items: center;\n      margin: 0.5rem 0;\n      height: 2rem; }\n\n.chart-header .general > div .label {\n        display: inline-block;\n        width: 9rem; }\n\n.chart-header .selected {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-orient: vertical;\n    -webkit-box-direction: normal;\n            flex-direction: column;\n    -webkit-box-align: end;\n            align-items: flex-end;\n    -webkit-box-pack: center;\n            justify-content: center;\n    -webkit-box-flex: 1;\n            flex-grow: 1;\n    font-size: 1.8rem; }\n\n.chart-header .selected span {\n      line-height: 2.9rem; }\n\n.chart {\n  position: relative;\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  -webkit-box-flex: 1;\n          flex: 1 1 auto;\n  min-height: 40rem; }\n\n.chart > div {\n    position: absolute;\n    width: 100%;\n    height: 100%; }\n\n.chart-options {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  height: 2.4rem;\n  -webkit-box-flex: 0;\n          flex: 0 0 auto; }\n\n.chart-options .title {\n    font-size: 1.3rem;\n    padding: 0 1rem; }\n\n.chart-options .title:first-child {\n      padding-left: 0; }\n\n.chart-options .options {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-pack: justify;\n            justify-content: space-between;\n    -webkit-box-flex: 1;\n            flex-grow: 1;\n    height: 100%; }\n\n.chart-options .options button {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-align: center;\n              align-items: center;\n      -webkit-box-pack: center;\n              justify-content: center;\n      -webkit-box-flex: 1;\n              flex: 1 1 auto;\n      cursor: pointer;\n      font-size: 1.3rem;\n      margin: 0 0.1rem;\n      padding: 0;\n      height: 100%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3N0YWtpbmcvc3Rha2luZy5jb21wb25lbnQuc2NzcyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiQUFBQTtFQUNFLG9CQUFhO0VBQWIsYUFBYTtFQUNiLDRCQUFzQjtFQUF0Qiw2QkFBc0I7VUFBdEIsc0JBQXNCO0VBQ3RCLFdBQVcsRUFBQTs7QUFHYjtFQUNFLG9CQUFhO0VBQWIsYUFBYTtFQUNiLG1CQUFjO1VBQWQsY0FBYyxFQUFBOztBQUZoQjtJQUtJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLDRCQUFzQjtJQUF0Qiw2QkFBc0I7WUFBdEIsc0JBQXNCO0lBQ3RCLHdCQUF1QjtZQUF2Qix1QkFBdUI7SUFDdkIsd0JBQXVCO1lBQXZCLHVCQUF1QjtJQUN2QixtQkFBWTtZQUFaLFlBQVk7SUFDWixpQkFBaUI7SUFDakIsaUJBQWlCLEVBQUE7O0FBWHJCO01BY00sb0JBQWE7TUFBYixhQUFhO01BQ2IseUJBQW1CO2NBQW5CLG1CQUFtQjtNQUNuQixnQkFBZ0I7TUFDaEIsWUFBWSxFQUFBOztBQWpCbEI7UUFvQlEscUJBQXFCO1FBQ3JCLFdBQVcsRUFBQTs7QUFyQm5CO0lBMkJJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLDRCQUFzQjtJQUF0Qiw2QkFBc0I7WUFBdEIsc0JBQXNCO0lBQ3RCLHNCQUFxQjtZQUFyQixxQkFBcUI7SUFDckIsd0JBQXVCO1lBQXZCLHVCQUF1QjtJQUN2QixtQkFBWTtZQUFaLFlBQVk7SUFDWixpQkFBaUIsRUFBQTs7QUFoQ3JCO01BbUNNLG1CQUFtQixFQUFBOztBQUt6QjtFQUNFLGtCQUFrQjtFQUNsQixvQkFBYTtFQUFiLGFBQWE7RUFDYix5QkFBbUI7VUFBbkIsbUJBQW1CO0VBQ25CLG1CQUFjO1VBQWQsY0FBYztFQUNkLGlCQUFpQixFQUFBOztBQUxuQjtJQVFJLGtCQUFrQjtJQUNsQixXQUFXO0lBQ1gsWUFBWSxFQUFBOztBQUloQjtFQUNFLG9CQUFhO0VBQWIsYUFBYTtFQUNiLHlCQUFtQjtVQUFuQixtQkFBbUI7RUFDbkIsY0FBYztFQUNkLG1CQUFjO1VBQWQsY0FBYyxFQUFBOztBQUpoQjtJQU9JLGlCQUFpQjtJQUNqQixlQUFlLEVBQUE7O0FBUm5CO01BV00sZUFBZSxFQUFBOztBQVhyQjtJQWdCSSxvQkFBYTtJQUFiLGFBQWE7SUFDYix5QkFBOEI7WUFBOUIsOEJBQThCO0lBQzlCLG1CQUFZO1lBQVosWUFBWTtJQUNaLFlBQVksRUFBQTs7QUFuQmhCO01Bc0JNLG9CQUFhO01BQWIsYUFBYTtNQUNiLHlCQUFtQjtjQUFuQixtQkFBbUI7TUFDbkIsd0JBQXVCO2NBQXZCLHVCQUF1QjtNQUN2QixtQkFBYztjQUFkLGNBQWM7TUFDZCxlQUFlO01BQ2YsaUJBQWlCO01BQ2pCLGdCQUFnQjtNQUNoQixVQUFVO01BQ1YsWUFBWSxFQUFBIiwiZmlsZSI6InNyYy9hcHAvc3Rha2luZy9zdGFraW5nLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xuICBkaXNwbGF5OiBmbGV4O1xuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xuICB3aWR0aDogMTAwJTtcbn1cblxuLmNoYXJ0LWhlYWRlciB7XG4gIGRpc3BsYXk6IGZsZXg7XG4gIGZsZXg6IDAgMCBhdXRvO1xuXG4gIC5nZW5lcmFsIHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XG4gICAgYWxpZ24taXRlbXM6IGZsZXgtc3RhcnQ7XG4gICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XG4gICAgZmxleC1ncm93OiAxO1xuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xuICAgIG1hcmdpbjogLTAuNXJlbSAwO1xuXG4gICAgPiBkaXYge1xuICAgICAgZGlzcGxheTogZmxleDtcbiAgICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG4gICAgICBtYXJnaW46IDAuNXJlbSAwO1xuICAgICAgaGVpZ2h0OiAycmVtO1xuXG4gICAgICAubGFiZWwge1xuICAgICAgICBkaXNwbGF5OiBpbmxpbmUtYmxvY2s7XG4gICAgICAgIHdpZHRoOiA5cmVtO1xuICAgICAgfVxuICAgIH1cbiAgfVxuXG4gIC5zZWxlY3RlZCB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xuICAgIGFsaWduLWl0ZW1zOiBmbGV4LWVuZDtcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcbiAgICBmbGV4LWdyb3c6IDE7XG4gICAgZm9udC1zaXplOiAxLjhyZW07XG5cbiAgICBzcGFuIHtcbiAgICAgIGxpbmUtaGVpZ2h0OiAyLjlyZW07XG4gICAgfVxuICB9XG59XG5cbi5jaGFydCB7XG4gIHBvc2l0aW9uOiByZWxhdGl2ZTtcbiAgZGlzcGxheTogZmxleDtcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgZmxleDogMSAxIGF1dG87XG4gIG1pbi1oZWlnaHQ6IDQwcmVtO1xuXG4gID4gZGl2IHtcbiAgICBwb3NpdGlvbjogYWJzb2x1dGU7XG4gICAgd2lkdGg6IDEwMCU7XG4gICAgaGVpZ2h0OiAxMDAlO1xuICB9XG59XG5cbi5jaGFydC1vcHRpb25zIHtcbiAgZGlzcGxheTogZmxleDtcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgaGVpZ2h0OiAyLjRyZW07XG4gIGZsZXg6IDAgMCBhdXRvO1xuXG4gIC50aXRsZSB7XG4gICAgZm9udC1zaXplOiAxLjNyZW07XG4gICAgcGFkZGluZzogMCAxcmVtO1xuXG4gICAgJjpmaXJzdC1jaGlsZHtcbiAgICAgIHBhZGRpbmctbGVmdDogMDtcbiAgICB9XG4gIH1cblxuICAub3B0aW9ucyB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XG4gICAgZmxleC1ncm93OiAxO1xuICAgIGhlaWdodDogMTAwJTtcblxuICAgIGJ1dHRvbiB7XG4gICAgICBkaXNwbGF5OiBmbGV4O1xuICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xuICAgICAgZmxleDogMSAxIGF1dG87XG4gICAgICBjdXJzb3I6IHBvaW50ZXI7XG4gICAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICAgIG1hcmdpbjogMCAwLjFyZW07XG4gICAgICBwYWRkaW5nOiAwO1xuICAgICAgaGVpZ2h0OiAxMDAlO1xuICAgIH1cbiAgfVxufVxuIl19 */"

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
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_7__ = __webpack_require__(/*! bignumber.js */ "./node_modules/bignumber.js/bignumber.js");
/* harmony import */ var bignumber_js__WEBPACK_IMPORTED_MODULE_7___default = /*#__PURE__*/__webpack_require__.n(bignumber_js__WEBPACK_IMPORTED_MODULE_7__);
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
    function StakingComponent(route, variablesService, backend, ngZone, intToMoneyPipe, translate) {
        this.route = route;
        this.variablesService = variablesService;
        this.backend = backend;
        this.ngZone = ngZone;
        this.intToMoneyPipe = intToMoneyPipe;
        this.translate = translate;
        this.periods = [
            {
                title: this.translate.instant('STAKING.PERIOD.WEEK1'),
                key: '1 week',
                active: false
            },
            {
                title: this.translate.instant('STAKING.PERIOD.WEEK2'),
                key: '2 week',
                active: false
            },
            {
                title: this.translate.instant('STAKING.PERIOD.MONTH1'),
                key: '1 month',
                active: false
            },
            {
                title: this.translate.instant('STAKING.PERIOD.MONTH3'),
                key: '3 month',
                active: false
            },
            {
                title: this.translate.instant('STAKING.PERIOD.MONTH6'),
                key: '6 month',
                active: false
            },
            {
                title: this.translate.instant('STAKING.PERIOD.YEAR'),
                key: '1 year',
                active: false
            },
            {
                title: this.translate.instant('STAKING.PERIOD.ALL'),
                key: 'All',
                active: true
            }
        ];
        this.groups = [
            {
                title: this.translate.instant('STAKING.GROUP.DAY'),
                key: 'day',
                active: true
            },
            {
                title: this.translate.instant('STAKING.GROUP.WEEK'),
                key: 'week',
                active: false
            },
            {
                title: this.translate.instant('STAKING.GROUP.MONTH'),
                key: 'month',
                active: false
            }
        ];
        this.selectedDate = {
            date: null,
            amount: null
        };
        this.originalData = [];
        this.total = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](0);
        this.pending = {
            list: [],
            total: new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](0)
        };
    }
    StakingComponent_1 = StakingComponent;
    StakingComponent.makeGroupTime = function (key, date) {
        if (key === 'day') {
            return date.setHours(0, 0, 0, 0);
        }
        else if (key === 'week') {
            return new Date(date.setDate(date.getDate() - date.getDay())).setHours(0, 0, 0, 0);
        }
        else {
            return new Date(date.setDate(1)).setHours(0, 0, 0, 0);
        }
    };
    StakingComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.parentRouting = this.route.parent.params.subscribe(function () {
            _this.getMiningHistory();
        });
        this.heightAppEvent = this.variablesService.getHeightAppEvent.subscribe(function (newHeight) {
            if (!_this.pending.total.isZero()) {
                var pendingCount = _this.pending.list.length;
                for (var i = pendingCount - 1; i >= 0; i--) {
                    if (newHeight - _this.pending.list[i].h >= 10) {
                        _this.pending.list.splice(i, 1);
                    }
                }
                if (pendingCount !== _this.pending.list.length) {
                    _this.pending.total = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](0);
                    for (var i = 0; i < _this.pending.list.length; i++) {
                        _this.pending.total = _this.pending.total.plus(_this.pending.list[i].a);
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
                zoomType: null,
                events: {
                    load: function () {
                        _this.changePeriod();
                    }
                }
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
                _this.total = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](0);
                _this.pending.list = [];
                _this.pending.total = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](0);
                _this.originalData = [];
                if (data.mined_entries) {
                    data.mined_entries.forEach(function (item, key) {
                        if (item.t.toString().length === 10) {
                            data.mined_entries[key].t = (new Date(item.t * 1000)).setUTCMilliseconds(0);
                        }
                    });
                    data.mined_entries.forEach(function (item) {
                        _this.total = _this.total.plus(item.a);
                        if (_this.variablesService.height_app - item.h < 10) {
                            _this.pending.list.push(item);
                            _this.pending.total = _this.pending.total.plus(item.a);
                        }
                        _this.originalData.push([parseInt(item.t, 10), parseFloat(_this.intToMoneyPipe.transform(item.a))]);
                    });
                    _this.originalData = _this.originalData.sort(function (a, b) {
                        return a[0] - b[0];
                    });
                }
                _this.ngZone.run(function () {
                    _this.drawChart([]);
                });
            });
        }
    };
    StakingComponent.prototype.changePeriod = function (period) {
        if (period) {
            this.periods.forEach(function (p) {
                p.active = false;
            });
            period.active = true;
        }
        else {
            period = this.periods.find(function (p) { return p.active; });
        }
        var d = new Date();
        var min = null;
        var newData = [];
        var group = this.groups.find(function (g) { return g.active; });
        if (period.key === '1 week') {
            this.originalData.forEach(function (item) {
                var time = StakingComponent_1.makeGroupTime(group.key, new Date(item[0]));
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](find[1]).plus(item[1]).toNumber();
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate() - 7, 0, 0, 0, 0);
        }
        else if (period.key === '2 week') {
            this.originalData.forEach(function (item) {
                var time = StakingComponent_1.makeGroupTime(group.key, new Date(item[0]));
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](find[1]).plus(item[1]).toNumber();
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate() - 14, 0, 0, 0, 0);
        }
        else if (period.key === '1 month') {
            this.originalData.forEach(function (item) {
                var time = StakingComponent_1.makeGroupTime(group.key, new Date(item[0]));
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](find[1]).plus(item[1]).toNumber();
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear(), d.getMonth() - 1, d.getDate(), 0, 0, 0, 0);
        }
        else if (period.key === '3 month') {
            this.originalData.forEach(function (item) {
                var time = StakingComponent_1.makeGroupTime(group.key, new Date(item[0]));
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](find[1]).plus(item[1]).toNumber();
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear(), d.getMonth() - 3, d.getDate(), 0, 0, 0, 0);
        }
        else if (period.key === '6 month') {
            this.originalData.forEach(function (item) {
                var time = StakingComponent_1.makeGroupTime(group.key, new Date(item[0]));
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](find[1]).plus(item[1]).toNumber();
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear(), d.getMonth() - 6, d.getDate(), 0, 0, 0, 0);
        }
        else if (period.key === '1 year') {
            this.originalData.forEach(function (item) {
                var time = StakingComponent_1.makeGroupTime(group.key, new Date(item[0]));
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](find[1]).plus(item[1]).toNumber();
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
            min = Date.UTC(d.getFullYear() - 1, d.getMonth(), d.getDate(), 0, 0, 0, 0);
        }
        else {
            this.originalData.forEach(function (item) {
                var time = StakingComponent_1.makeGroupTime(group.key, new Date(item[0]));
                var find = newData.find(function (itemNew) { return itemNew[0] === time; });
                if (find) {
                    find[1] = new bignumber_js__WEBPACK_IMPORTED_MODULE_7__["BigNumber"](find[1]).plus(item[1]).toNumber();
                }
                else {
                    newData.push([time, item[1]]);
                }
            });
            this.chart.ref.series[0].setData(newData, true);
        }
        this.chart.ref.xAxis[0].setExtremes(min, null);
    };
    StakingComponent.prototype.changeGroup = function (group) {
        this.groups.forEach(function (g) {
            g.active = false;
        });
        group.active = true;
        this.changePeriod();
    };
    StakingComponent.prototype.ngOnDestroy = function () {
        this.parentRouting.unsubscribe();
        this.heightAppEvent.unsubscribe();
        this.refreshStackingEvent.unsubscribe();
    };
    var StakingComponent_1;
    StakingComponent = StakingComponent_1 = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-staking',
            template: __webpack_require__(/*! ./staking.component.html */ "./src/app/staking/staking.component.html"),
            styles: [__webpack_require__(/*! ./staking.component.scss */ "./src/app/staking/staking.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_4__["ActivatedRoute"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_1__["VariablesService"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_5__["IntToMoneyPipe"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_6__["TranslateService"]])
    ], StakingComponent);
    return StakingComponent;
}());



/***/ }),

/***/ "./src/app/transfer-alias/transfer-alias.component.html":
/*!**************************************************************!*\
  !*** ./src/app/transfer-alias/transfer-alias.component.html ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content\">\n\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span [routerLink]=\"['/wallet/' + wallet.wallet_id + '/history']\">{{ wallet.name }}</span>\n      <span>{{ 'BREADCRUMBS.TRANSFER_ALIAS' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <form class=\"form-transfer\">\n\n    <div class=\"input-block alias-name\">\n      <label for=\"alias-name\">\n        {{ 'EDIT_ALIAS.NAME.LABEL' | translate }}\n      </label>\n      <input type=\"text\" id=\"alias-name\" [value]=\"alias.name\" placeholder=\"{{ 'EDIT_ALIAS.NAME.PLACEHOLDER' | translate }}\" readonly>\n    </div>\n\n    <div class=\"input-block textarea\">\n      <label for=\"alias-comment\">\n        {{ 'EDIT_ALIAS.COMMENT.LABEL' | translate }}\n      </label>\n      <textarea id=\"alias-comment\" [value]=\"alias.comment\" placeholder=\"{{ 'EDIT_ALIAS.COMMENT.PLACEHOLDER' | translate }}\" readonly></textarea>\n    </div>\n\n    <div class=\"input-block alias-transfer-address\">\n      <label for=\"alias-transfer\">\n        {{ 'TRANSFER_ALIAS.ADDRESS.LABEL' | translate }}\n      </label>\n      <input type=\"text\" id=\"alias-transfer\" [(ngModel)]=\"transferAddress\" [ngModelOptions]=\"{standalone: true}\" (ngModelChange)=\"changeAddress()\" placeholder=\"{{ 'TRANSFER_ALIAS.ADDRESS.PLACEHOLDER' | translate }}\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"transferAddress.length > 0 && (transferAddressAlias || !transferAddressValid || (transferAddressValid && !permissionSend) || notEnoughMoney)\">\n        <div *ngIf=\"!transferAddressValid\">\n          {{ 'TRANSFER_ALIAS.FORM_ERRORS.WRONG_ADDRESS' | translate }}\n        </div>\n        <div *ngIf=\"transferAddressAlias || (transferAddressValid && !permissionSend)\">\n          {{ 'TRANSFER_ALIAS.FORM_ERRORS.ALIAS_EXISTS' | translate }}\n        </div>\n        <div *ngIf=\"notEnoughMoney\">\n          {{ 'TRANSFER_ALIAS.FORM_ERRORS.NO_MONEY' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"alias-cost\">{{ \"TRANSFER_ALIAS.COST\" | translate : {value: variablesService.default_fee, currency: variablesService.defaultCurrency} }}</div>\n\n    <div class=\"wrap-buttons\">\n      <button type=\"button\" class=\"blue-button\" (click)=\"transferAlias()\" [disabled]=\"transferAddressAlias || !transferAddressValid || notEnoughMoney\">{{ 'TRANSFER_ALIAS.BUTTON_TRANSFER' | translate }}</button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"back()\">{{ 'TRANSFER_ALIAS.BUTTON_CANCEL' | translate }}</button>\n    </div>\n\n  </form>\n\n</div>\n"

/***/ }),

/***/ "./src/app/transfer-alias/transfer-alias.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/transfer-alias/transfer-alias.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-transfer {\n  margin: 2.4rem 0; }\n  .form-transfer .alias-name {\n    width: 50%; }\n  .form-transfer .alias-cost {\n    font-size: 1.3rem;\n    margin-top: 2rem; }\n  .form-transfer .wrap-buttons {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-pack: justify;\n            justify-content: space-between;\n    margin: 2.5rem -0.7rem; }\n  .form-transfer .wrap-buttons button {\n      margin: 0 0.7rem;\n      width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3RyYW5zZmVyLWFsaWFzL3RyYW5zZmVyLWFsaWFzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZ0JBQWdCLEVBQUE7RUFEbEI7SUFJSSxVQUFVLEVBQUE7RUFKZDtJQVFJLGlCQUFpQjtJQUNqQixnQkFBZ0IsRUFBQTtFQVRwQjtJQWFJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLHlCQUE4QjtZQUE5Qiw4QkFBOEI7SUFDOUIsc0JBQXNCLEVBQUE7RUFmMUI7TUFrQk0sZ0JBQWdCO01BQ2hCLFlBQVksRUFBQSIsImZpbGUiOiJzcmMvYXBwL3RyYW5zZmVyLWFsaWFzL3RyYW5zZmVyLWFsaWFzLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tdHJhbnNmZXIge1xuICBtYXJnaW46IDIuNHJlbSAwO1xuXG4gIC5hbGlhcy1uYW1lIHtcbiAgICB3aWR0aDogNTAlO1xuICB9XG5cbiAgLmFsaWFzLWNvc3Qge1xuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xuICAgIG1hcmdpbi10b3A6IDJyZW07XG4gIH1cblxuICAud3JhcC1idXR0b25zIHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcbiAgICBtYXJnaW46IDIuNXJlbSAtMC43cmVtO1xuXG4gICAgYnV0dG9uIHtcbiAgICAgIG1hcmdpbjogMCAwLjdyZW07XG4gICAgICB3aWR0aDogMTVyZW07XG4gICAgfVxuICB9XG59XG4iXX0= */"

/***/ }),

/***/ "./src/app/transfer-alias/transfer-alias.component.ts":
/*!************************************************************!*\
  !*** ./src/app/transfer-alias/transfer-alias.component.ts ***!
  \************************************************************/
/*! exports provided: TransferAliasComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "TransferAliasComponent", function() { return TransferAliasComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};






var TransferAliasComponent = /** @class */ (function () {
    function TransferAliasComponent(location, router, backend, variablesService, modalService, ngZone) {
        this.location = location;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.transferAddress = '';
        this.requestProcessing = false;
    }
    TransferAliasComponent.prototype.ngOnInit = function () {
        this.wallet = this.variablesService.currentWallet;
        var alias = this.backend.getWalletAlias(this.wallet.address);
        this.alias = {
            name: alias.name,
            address: alias.address,
            comment: alias.comment,
            tracking_key: alias.tracking_key
        };
        this.notEnoughMoney = this.wallet.unlocked_balance.isLessThan(this.variablesService.default_fee_big);
    };
    TransferAliasComponent.prototype.changeAddress = function () {
        var _this = this;
        this.backend.validateAddress(this.transferAddress, function (status) {
            _this.transferAddressValid = status;
            if (status) {
                _this.backend.getPoolInfo(function (statusPool, dataPool) {
                    if (dataPool.hasOwnProperty('aliases_que') && dataPool.aliases_que.length) {
                        _this.setStatus(!dataPool.aliases_que.some(function (el) { return el.address === _this.transferAddress; }));
                    }
                    else {
                        _this.setStatus(status);
                    }
                });
            }
            else {
                _this.setStatus(false);
            }
        });
    };
    TransferAliasComponent.prototype.setStatus = function (statusSet) {
        var _this = this;
        this.permissionSend = statusSet;
        if (statusSet) {
            this.backend.getAliasByAddress(this.transferAddress, function (status) {
                _this.ngZone.run(function () {
                    if (status) {
                        _this.transferAddressAlias = true;
                        _this.permissionSend = false;
                    }
                    else {
                        _this.transferAddressAlias = false;
                    }
                });
            });
        }
        else {
            this.ngZone.run(function () {
                _this.transferAddressAlias = false;
            });
        }
    };
    TransferAliasComponent.prototype.transferAlias = function () {
        var _this = this;
        if (this.requestProcessing || !this.permissionSend || !this.transferAddressValid || this.notEnoughMoney) {
            return;
        }
        this.requestProcessing = true;
        var newAlias = {
            name: this.alias.name,
            address: this.transferAddress,
            comment: this.alias.comment,
            tracking_key: this.alias.tracking_key
        };
        this.backend.updateAlias(this.wallet.wallet_id, newAlias, this.variablesService.default_fee, function (status, data) {
            if (status && data.hasOwnProperty('success') && data.success) {
                _this.modalService.prepareModal('info', 'TRANSFER_ALIAS.REQUEST_SEND_REG');
                _this.ngZone.run(function () {
                    _this.router.navigate(['/wallet/' + _this.wallet.wallet_id]);
                });
            }
            _this.requestProcessing = false;
        });
    };
    TransferAliasComponent.prototype.back = function () {
        this.location.back();
    };
    TransferAliasComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-transfer-alias',
            template: __webpack_require__(/*! ./transfer-alias.component.html */ "./src/app/transfer-alias/transfer-alias.component.html"),
            styles: [__webpack_require__(/*! ./transfer-alias.component.scss */ "./src/app/transfer-alias/transfer-alias.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_common__WEBPACK_IMPORTED_MODULE_1__["Location"],
            _angular_router__WEBPACK_IMPORTED_MODULE_2__["Router"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_4__["VariablesService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_5__["ModalService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"]])
    ], TransferAliasComponent);
    return TransferAliasComponent;
}());



/***/ }),

/***/ "./src/app/typing-message/typing-message.component.html":
/*!**************************************************************!*\
  !*** ./src/app/typing-message/typing-message.component.html ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"head\">\n  <div class=\"interlocutor\">\n    @bitmain\n  </div>\n  <a class=\"back-btn\" [routerLink]=\"['/main']\">\n    <i class=\"icon back\"></i>\n    <span>{{ 'COMMON.BACK' | translate }}</span>\n  </a>\n</div>\n\n<div class=\"messages-content\">\n  <div class=\"messages-list scrolled-content\">\n    <div class=\"date\">10:39</div>\n    <div class=\"my\">\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n    </div>\n    <div class=\"buddy\">\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n    </div>\n    <div class=\"my\">\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n    </div>\n    <div class=\"buddy\">\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n    </div>\n    <div class=\"date\">11:44</div>\n    <div class=\"my\">\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n    </div>\n    <div class=\"buddy\">\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n    </div>\n    <div class=\"my\">\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n    </div>\n    <div class=\"date\">12:15</div>\n    <div class=\"my\">\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n    </div>\n    <div class=\"buddy\">\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n    </div>\n    <div class=\"my\">\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n    </div>\n    <div class=\"date\">13:13</div>\n    <div class=\"my\">\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n    </div>\n    <div class=\"buddy\">\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\n    </div>\n    <div class=\"my\">\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n    </div>\n  </div>\n  <div class=\"type-message\">\n    <div class=\"input-block textarea\">\n      <textarea placeholder=\"{{ 'MESSAGES.SEND_PLACEHOLDER' | translate }}\"></textarea>\n    </div>\n    <button type=\"button\" class=\"blue-button\">{{ 'MESSAGES.SEND_BUTTON' | translate }}</button>\n  </div>\n</div>\n"

/***/ }),

/***/ "./src/app/typing-message/typing-message.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/typing-message/typing-message.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  width: 100%; }\n\n.head {\n  -webkit-box-flex: 0;\n          flex: 0 0 auto;\n  box-sizing: content-box;\n  margin: -3rem -3rem 0; }\n\n.messages-content {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  -webkit-box-pack: justify;\n          justify-content: space-between;\n  -webkit-box-flex: 1;\n          flex-grow: 1; }\n\n.messages-content .messages-list {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-orient: vertical;\n    -webkit-box-direction: normal;\n            flex-direction: column;\n    font-size: 1.3rem;\n    margin: 1rem -3rem;\n    padding: 0 3rem;\n    overflow-y: overlay; }\n\n.messages-content .messages-list div {\n      margin: 0.7rem 0; }\n\n.messages-content .messages-list div.date {\n        text-align: center; }\n\n.messages-content .messages-list div.my, .messages-content .messages-list div.buddy {\n        position: relative;\n        padding: 1.8rem;\n        max-width: 60%; }\n\n.messages-content .messages-list div.buddy {\n        align-self: flex-end; }\n\n.messages-content .type-message {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-flex: 0;\n            flex: 0 0 auto;\n    width: 100%;\n    height: 4.2rem; }\n\n.messages-content .type-message .input-block {\n      width: 100%; }\n\n.messages-content .type-message .input-block > textarea {\n        min-height: 4.2rem; }\n\n.messages-content .type-message button {\n      -webkit-box-flex: 0;\n              flex: 0 0 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3R5cGluZy1tZXNzYWdlL3R5cGluZy1tZXNzYWdlLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usb0JBQWE7RUFBYixhQUFhO0VBQ2IsNEJBQXNCO0VBQXRCLDZCQUFzQjtVQUF0QixzQkFBc0I7RUFDdEIsV0FBVyxFQUFBOztBQUdiO0VBQ0UsbUJBQWM7VUFBZCxjQUFjO0VBQ2QsdUJBQXVCO0VBQ3ZCLHFCQUFxQixFQUFBOztBQUd2QjtFQUNFLG9CQUFhO0VBQWIsYUFBYTtFQUNiLDRCQUFzQjtFQUF0Qiw2QkFBc0I7VUFBdEIsc0JBQXNCO0VBQ3RCLHlCQUE4QjtVQUE5Qiw4QkFBOEI7RUFDOUIsbUJBQVk7VUFBWixZQUFZLEVBQUE7O0FBSmQ7SUFPSSxvQkFBYTtJQUFiLGFBQWE7SUFDYiw0QkFBc0I7SUFBdEIsNkJBQXNCO1lBQXRCLHNCQUFzQjtJQUN0QixpQkFBaUI7SUFDakIsa0JBQWtCO0lBQ2xCLGVBQWU7SUFDZixtQkFBbUIsRUFBQTs7QUFadkI7TUFlTSxnQkFBZ0IsRUFBQTs7QUFmdEI7UUFrQlEsa0JBQWtCLEVBQUE7O0FBbEIxQjtRQXNCUSxrQkFBa0I7UUFDbEIsZUFBZTtRQUNmLGNBQWMsRUFBQTs7QUF4QnRCO1FBNEJRLG9CQUFvQixFQUFBOztBQTVCNUI7SUFrQ0ksb0JBQWE7SUFBYixhQUFhO0lBQ2IsbUJBQWM7WUFBZCxjQUFjO0lBQ2QsV0FBVztJQUNYLGNBQWMsRUFBQTs7QUFyQ2xCO01Bd0NNLFdBQVcsRUFBQTs7QUF4Q2pCO1FBMkNRLGtCQUFrQixFQUFBOztBQTNDMUI7TUFnRE0sbUJBQWU7Y0FBZixlQUFlLEVBQUEiLCJmaWxlIjoic3JjL2FwcC90eXBpbmctbWVzc2FnZS90eXBpbmctbWVzc2FnZS5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcbiAgZGlzcGxheTogZmxleDtcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcbiAgd2lkdGg6IDEwMCU7XG59XG5cbi5oZWFkIHtcbiAgZmxleDogMCAwIGF1dG87XG4gIGJveC1zaXppbmc6IGNvbnRlbnQtYm94O1xuICBtYXJnaW46IC0zcmVtIC0zcmVtIDA7XG59XG5cbi5tZXNzYWdlcy1jb250ZW50IHtcbiAgZGlzcGxheTogZmxleDtcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcbiAganVzdGlmeS1jb250ZW50OiBzcGFjZS1iZXR3ZWVuO1xuICBmbGV4LWdyb3c6IDE7XG5cbiAgLm1lc3NhZ2VzLWxpc3Qge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcbiAgICBmb250LXNpemU6IDEuM3JlbTtcbiAgICBtYXJnaW46IDFyZW0gLTNyZW07XG4gICAgcGFkZGluZzogMCAzcmVtO1xuICAgIG92ZXJmbG93LXk6IG92ZXJsYXk7XG5cbiAgICBkaXYge1xuICAgICAgbWFyZ2luOiAwLjdyZW0gMDtcblxuICAgICAgJi5kYXRlIHtcbiAgICAgICAgdGV4dC1hbGlnbjogY2VudGVyO1xuICAgICAgfVxuXG4gICAgICAmLm15LCAmLmJ1ZGR5IHtcbiAgICAgICAgcG9zaXRpb246IHJlbGF0aXZlO1xuICAgICAgICBwYWRkaW5nOiAxLjhyZW07XG4gICAgICAgIG1heC13aWR0aDogNjAlO1xuICAgICAgfVxuXG4gICAgICAmLmJ1ZGR5IHtcbiAgICAgICAgYWxpZ24tc2VsZjogZmxleC1lbmQ7XG4gICAgICB9XG4gICAgfVxuICB9XG5cbiAgLnR5cGUtbWVzc2FnZSB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBmbGV4OiAwIDAgYXV0bztcbiAgICB3aWR0aDogMTAwJTtcbiAgICBoZWlnaHQ6IDQuMnJlbTtcblxuICAgIC5pbnB1dC1ibG9jayB7XG4gICAgICB3aWR0aDogMTAwJTtcblxuICAgICAgPiB0ZXh0YXJlYSB7XG4gICAgICAgIG1pbi1oZWlnaHQ6IDQuMnJlbTtcbiAgICAgIH1cbiAgICB9XG5cbiAgICBidXR0b24ge1xuICAgICAgZmxleDogMCAwIDE1cmVtO1xuICAgIH1cbiAgfVxufVxuXG4iXX0= */"

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

module.exports = "<div class=\"content\">\n\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span (click)=\"back()\">{{variablesService.currentWallet.name}}</span>\n      <span>{{ 'BREADCRUMBS.WALLET_DETAILS' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <form class=\"form-details\" [formGroup]=\"detailsForm\" (ngSubmit)=\"onSubmitEdit()\">\n\n    <div class=\"input-block\">\n      <label for=\"wallet-name\">{{ 'WALLET_DETAILS.LABEL_NAME' | translate }}</label>\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [maxLength]=\"variablesService.maxWalletNameLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"detailsForm.controls['name'].invalid && (detailsForm.controls['name'].dirty || detailsForm.controls['name'].touched)\">\n        <div *ngIf=\"detailsForm.controls['name'].errors['required']\">\n          {{ 'WALLET_DETAILS.FORM_ERRORS.NAME_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"detailsForm.controls['name'].errors['duplicate']\">\n          {{ 'WALLET_DETAILS.FORM_ERRORS.NAME_DUPLICATE' | translate }}\n        </div>\n      </div>\n      <div class=\"error-block\" *ngIf=\"detailsForm.get('name').value.length >= variablesService.maxWalletNameLength\">\n        {{ 'WALLET_DETAILS.FORM_ERRORS.MAX_LENGTH' | translate }}\n      </div>\n    </div>\n\n    <div class=\"input-block\">\n      <label for=\"wallet-location\">{{ 'WALLET_DETAILS.LABEL_FILE_LOCATION' | translate }}</label>\n      <input type=\"text\" id=\"wallet-location\" formControlName=\"path\" readonly>\n    </div>\n\n    <div class=\"input-block textarea\">\n      <label for=\"seed-phrase\">{{ 'WALLET_DETAILS.LABEL_SEED_PHRASE' | translate }}</label>\n      <div class=\"seed-phrase\" id=\"seed-phrase\">\n        <div class=\"seed-phrase-hint\" (click)=\"showSeedPhrase()\" *ngIf=\"!showSeed\">{{ 'WALLET_DETAILS.SEED_PHRASE_HINT' | translate }}</div>\n        <div class=\"seed-phrase-content\" *ngIf=\"showSeed\" (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, seedPhrase)\">\n          <ng-container *ngFor=\"let word of seedPhrase.split(' '); let index = index\">\n            <div class=\"word\">{{(index + 1) + '. ' + word}}</div>\n          </ng-container>\n        </div>\n      </div>\n    </div>\n\n    <div class=\"wallet-buttons\">\n      <button type=\"submit\" class=\"blue-button\" [disabled]=\"!detailsForm.valid\">{{ 'WALLET_DETAILS.BUTTON_SAVE' | translate }}</button>\n      <button type=\"button\" class=\"blue-button\" (click)=\"closeWallet()\">{{ 'WALLET_DETAILS.BUTTON_REMOVE' | translate }}</button>\n    </div>\n\n  </form>\n\n</div>\n"

/***/ }),

/***/ "./src/app/wallet-details/wallet-details.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/wallet-details/wallet-details.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-details {\n  margin-top: 1.8rem; }\n  .form-details .input-block:first-child {\n    width: 50%; }\n  .form-details .seed-phrase {\n    display: -webkit-box;\n    display: flex;\n    font-size: 1.4rem;\n    line-height: 1.5rem;\n    padding: 1.4rem;\n    width: 100%;\n    height: 8.8rem; }\n  .form-details .seed-phrase .seed-phrase-hint {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-align: center;\n              align-items: center;\n      -webkit-box-pack: center;\n              justify-content: center;\n      cursor: pointer;\n      width: 100%;\n      height: 100%; }\n  .form-details .seed-phrase .seed-phrase-content {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-orient: vertical;\n      -webkit-box-direction: normal;\n              flex-direction: column;\n      flex-wrap: wrap;\n      width: 100%;\n      height: 100%; }\n  .form-details .wallet-buttons {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    -webkit-box-pack: justify;\n            justify-content: space-between; }\n  .form-details .wallet-buttons button {\n      margin: 2.9rem 0;\n      width: 100%;\n      max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3dhbGxldC1kZXRhaWxzL3dhbGxldC1kZXRhaWxzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCLEVBQUE7RUFEcEI7SUFNTSxVQUFVLEVBQUE7RUFOaEI7SUFXSSxvQkFBYTtJQUFiLGFBQWE7SUFDYixpQkFBaUI7SUFDakIsbUJBQW1CO0lBQ25CLGVBQWU7SUFDZixXQUFXO0lBQ1gsY0FBYyxFQUFBO0VBaEJsQjtNQW1CTSxvQkFBYTtNQUFiLGFBQWE7TUFDYix5QkFBbUI7Y0FBbkIsbUJBQW1CO01BQ25CLHdCQUF1QjtjQUF2Qix1QkFBdUI7TUFDdkIsZUFBZTtNQUNmLFdBQVc7TUFDWCxZQUFZLEVBQUE7RUF4QmxCO01BNEJNLG9CQUFhO01BQWIsYUFBYTtNQUNiLDRCQUFzQjtNQUF0Qiw2QkFBc0I7Y0FBdEIsc0JBQXNCO01BQ3RCLGVBQWU7TUFDZixXQUFXO01BQ1gsWUFBWSxFQUFBO0VBaENsQjtJQXFDSSxvQkFBYTtJQUFiLGFBQWE7SUFDYix5QkFBbUI7WUFBbkIsbUJBQW1CO0lBQ25CLHlCQUE4QjtZQUE5Qiw4QkFBOEIsRUFBQTtFQXZDbEM7TUEwQ00sZ0JBQWdCO01BQ2hCLFdBQVc7TUFDWCxnQkFBZ0IsRUFBQSIsImZpbGUiOiJzcmMvYXBwL3dhbGxldC1kZXRhaWxzL3dhbGxldC1kZXRhaWxzLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tZGV0YWlscyB7XG4gIG1hcmdpbi10b3A6IDEuOHJlbTtcblxuICAuaW5wdXQtYmxvY2sge1xuXG4gICAgJjpmaXJzdC1jaGlsZCB7XG4gICAgICB3aWR0aDogNTAlO1xuICAgIH1cbiAgfVxuXG4gIC5zZWVkLXBocmFzZSB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBmb250LXNpemU6IDEuNHJlbTtcbiAgICBsaW5lLWhlaWdodDogMS41cmVtO1xuICAgIHBhZGRpbmc6IDEuNHJlbTtcbiAgICB3aWR0aDogMTAwJTtcbiAgICBoZWlnaHQ6IDguOHJlbTtcblxuICAgIC5zZWVkLXBocmFzZS1oaW50IHtcbiAgICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XG4gICAgICBjdXJzb3I6IHBvaW50ZXI7XG4gICAgICB3aWR0aDogMTAwJTtcbiAgICAgIGhlaWdodDogMTAwJTtcbiAgICB9XG5cbiAgICAuc2VlZC1waHJhc2UtY29udGVudCB7XG4gICAgICBkaXNwbGF5OiBmbGV4O1xuICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcbiAgICAgIGZsZXgtd3JhcDogd3JhcDtcbiAgICAgIHdpZHRoOiAxMDAlO1xuICAgICAgaGVpZ2h0OiAxMDAlO1xuICAgIH1cbiAgfVxuXG4gIC53YWxsZXQtYnV0dG9ucyB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcblxuICAgIGJ1dHRvbiB7XG4gICAgICBtYXJnaW46IDIuOXJlbSAwO1xuICAgICAgd2lkdGg6IDEwMCU7XG4gICAgICBtYXgtd2lkdGg6IDE1cmVtO1xuICAgIH1cbiAgfVxuXG59XG4iXX0= */"

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
                        if (g.value === _this.variablesService.wallets[i].name) {
                            if (_this.variablesService.wallets[i].wallet_id === _this.variablesService.currentWallet.wallet_id) {
                                return { 'same': true };
                            }
                            else {
                                return { 'duplicate': true };
                            }
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
        this.backend.getSmartWalletInfo(this.variablesService.currentWallet.wallet_id, function (status, data) {
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
        var _this = this;
        if (this.detailsForm.value) {
            this.variablesService.currentWallet.name = this.detailsForm.get('name').value;
            this.ngZone.run(function () {
                _this.router.navigate(['/wallet/' + _this.variablesService.currentWallet.wallet_id]);
            });
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
            _this.ngZone.run(function () {
                if (_this.variablesService.wallets.length) {
                    _this.variablesService.currentWallet = _this.variablesService.wallets[0];
                    _this.router.navigate(['/wallet/' + _this.variablesService.currentWallet.wallet_id]);
                }
                else {
                    _this.router.navigate(['/']);
                }
            });
            if (_this.variablesService.appPass) {
                _this.backend.storeSecureAppData();
            }
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

module.exports = "<div class=\"header\">\n  <div>\n    <h3 tooltip=\"{{ variablesService.currentWallet.name }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{variablesService.currentWallet.name}}</h3>\n    <button [routerLink]=\"['/assign-alias']\" *ngIf=\"!variablesService.currentWallet.alias.hasOwnProperty('name') && variablesService.currentWallet.loaded && variablesService.daemon_state === 2 && variablesService.currentWallet.alias_available\">\n      <i class=\"icon account\"></i>\n      <span>{{ 'WALLET.REGISTER_ALIAS' | translate }}</span>\n    </button>\n    <div class=\"alias\" *ngIf=\"variablesService.currentWallet.alias.hasOwnProperty('name') && variablesService.currentWallet.loaded && variablesService.daemon_state === 2\">\n      <span>{{variablesService.currentWallet.alias['name']}}</span>\n      <ng-container *ngIf=\"variablesService.currentWallet.alias_available\">\n        <i class=\"icon edit\" [routerLink]=\"['/edit-alias']\"></i>\n        <i class=\"icon transfer\" [routerLink]=\"['/transfer-alias']\"></i>\n      </ng-container>\n    </div>\n  </div>\n  <div>\n    <button [routerLink]=\"['/details']\" routerLinkActive=\"active\">\n      <i class=\"icon details\"></i>\n      <span>{{ 'WALLET.DETAILS' | translate }}</span>\n    </button>\n  </div>\n</div>\n<div class=\"address\">\n  <span>{{variablesService.currentWallet.address}}</span>\n  <i class=\"icon\" [class.copy]=\"!copyAnimation\" [class.copied]=\"copyAnimation\" (click)=\"copyAddress()\"></i>\n</div>\n<div class=\"balance\">\n  <span [tooltip]=\"getTooltip()\" [placement]=\"'bottom'\" [tooltipClass]=\"'balance-tooltip'\" [delay]=\"150\" [timeout]=\"0\" (onHide)=\"onHideTooltip()\">{{variablesService.currentWallet.balance | intToMoney  : '3'}} {{variablesService.defaultCurrency}}</span>\n  <span>$ {{variablesService.currentWallet.getMoneyEquivalent(variablesService.moneyEquivalent) | intToMoney | number : '1.2-2'}}</span>\n</div>\n<div class=\"tabs\">\n  <div class=\"tabs-header\">\n    <ng-container *ngFor=\"let tab of tabs; let index = index\">\n      <div class=\"tab\" [class.active]=\"tab.active\" [class.disabled]=\"(tab.link === '/send' || tab.link === '/contracts' || tab.link === '/staking') && (variablesService.daemon_state !== 2 || !variablesService.currentWallet.loaded)\" (click)=\"changeTab(index)\">\n        <i class=\"icon\" [ngClass]=\"tab.icon\"></i>\n        <span>{{ tab.title | translate }}</span>\n        <span class=\"indicator\" *ngIf=\"tab.indicator\">{{variablesService.currentWallet.new_contracts}}</span>\n      </div>\n    </ng-container>\n  </div>\n  <div #scrolledContent class=\"tabs-content scrolled-content\">\n    <router-outlet></router-outlet>\n  </div>\n</div>\n\n"

/***/ }),

/***/ "./src/app/wallet/wallet.component.scss":
/*!**********************************************!*\
  !*** ./src/app/wallet/wallet.component.scss ***!
  \**********************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: relative;\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  padding: 0 3rem 3rem;\n  min-width: 95rem;\n  width: 100%;\n  height: 100%; }\n\n.header {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  -webkit-box-pack: justify;\n          justify-content: space-between;\n  -webkit-box-flex: 0;\n          flex: 0 0 auto;\n  height: 8rem; }\n\n.header > div {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center; }\n\n.header > div :not(:last-child) {\n      margin-right: 3.2rem; }\n\n.header h3 {\n    font-size: 1.7rem;\n    font-weight: 600;\n    text-overflow: ellipsis;\n    overflow: hidden;\n    white-space: nowrap;\n    max-width: 50rem;\n    line-height: 2.7rem; }\n\n.header button {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    background: transparent;\n    border: none;\n    cursor: pointer;\n    font-weight: 400;\n    outline: none;\n    padding: 0; }\n\n.header button .icon {\n      margin-right: 1.2rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.header button .icon.account {\n        -webkit-mask: url('account.svg') no-repeat center;\n                mask: url('account.svg') no-repeat center; }\n\n.header button .icon.details {\n        -webkit-mask: url('details.svg') no-repeat center;\n                mask: url('details.svg') no-repeat center; }\n\n.header button .icon.lock {\n        -webkit-mask: url('lock.svg') no-repeat center;\n                mask: url('lock.svg') no-repeat center; }\n\n.header .alias {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-align: center;\n            align-items: center;\n    font-size: 1.3rem; }\n\n.header .alias .icon {\n      cursor: pointer;\n      margin-right: 1.2rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.header .alias .icon.edit {\n        -webkit-mask: url('details.svg') no-repeat center;\n                mask: url('details.svg') no-repeat center; }\n\n.header .alias .icon.transfer {\n        -webkit-mask: url('send.svg') no-repeat center;\n                mask: url('send.svg') no-repeat center; }\n\n.address {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: center;\n          align-items: center;\n  -webkit-box-flex: 0;\n          flex: 0 0 auto;\n  font-size: 1.4rem;\n  line-height: 1.7rem; }\n\n.address .icon {\n    cursor: pointer;\n    margin-left: 1.2rem;\n    width: 1.7rem;\n    height: 1.7rem; }\n\n.address .icon.copy {\n      -webkit-mask: url('copy.svg') no-repeat center;\n              mask: url('copy.svg') no-repeat center; }\n\n.address .icon.copy:hover {\n        opacity: 0.75; }\n\n.address .icon.copied {\n      -webkit-mask: url('complete-testwallet.svg') no-repeat center;\n              mask: url('complete-testwallet.svg') no-repeat center; }\n\n.balance {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-align: end;\n          align-items: flex-end;\n  -webkit-box-pack: start;\n          justify-content: flex-start;\n  -webkit-box-flex: 0;\n          flex: 0 0 auto;\n  margin: 2.6rem 0; }\n\n.balance :first-child {\n    font-size: 3.3rem;\n    font-weight: 600;\n    line-height: 2.4rem;\n    margin-right: 3.5rem; }\n\n.balance :last-child {\n    font-size: 1.8rem;\n    font-weight: 600;\n    line-height: 1.3rem; }\n\n.tabs {\n  display: -webkit-box;\n  display: flex;\n  -webkit-box-orient: vertical;\n  -webkit-box-direction: normal;\n          flex-direction: column;\n  -webkit-box-flex: 1;\n          flex: 1 1 auto; }\n\n.tabs .tabs-header {\n    display: -webkit-box;\n    display: flex;\n    -webkit-box-pack: justify;\n            justify-content: space-between;\n    -webkit-box-flex: 0;\n            flex: 0 0 auto; }\n\n.tabs .tabs-header .tab {\n      display: -webkit-box;\n      display: flex;\n      -webkit-box-align: center;\n              align-items: center;\n      -webkit-box-pack: center;\n              justify-content: center;\n      -webkit-box-flex: 1;\n              flex: 1 0 auto;\n      cursor: pointer;\n      padding: 0 1rem;\n      height: 5rem; }\n\n.tabs .tabs-header .tab .icon {\n        margin-right: 1.3rem;\n        width: 1.7rem;\n        height: 1.7rem; }\n\n.tabs .tabs-header .tab .icon.send {\n          -webkit-mask: url('send.svg') no-repeat center;\n                  mask: url('send.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.receive {\n          -webkit-mask: url('receive.svg') no-repeat center;\n                  mask: url('receive.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.history {\n          -webkit-mask: url('history.svg') no-repeat center;\n                  mask: url('history.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.contracts {\n          -webkit-mask: url('contracts.svg') no-repeat center;\n                  mask: url('contracts.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.messages {\n          -webkit-mask: url('message.svg') no-repeat center;\n                  mask: url('message.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.staking {\n          -webkit-mask: url('staking.svg') no-repeat center;\n                  mask: url('staking.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .indicator {\n        display: -webkit-box;\n        display: flex;\n        -webkit-box-align: center;\n                align-items: center;\n        -webkit-box-pack: center;\n                justify-content: center;\n        border-radius: 1rem;\n        font-size: 1rem;\n        font-weight: 600;\n        margin-left: 1.3rem;\n        padding: 0 0.5rem;\n        min-width: 1.6rem;\n        height: 1.6rem; }\n\n.tabs .tabs-header .tab.disabled {\n        cursor: url('not-allowed.svg'), not-allowed; }\n\n.tabs .tabs-header .tab:not(:last-child) {\n        margin-right: 0.3rem; }\n\n.tabs .tabs-content {\n    display: -webkit-box;\n    display: flex;\n    padding: 3rem;\n    -webkit-box-flex: 1;\n            flex: 1 1 auto;\n    overflow-x: hidden;\n    overflow-y: overlay; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi9Vc2Vycy9hcHBsZS9Eb2N1bWVudHMvemFuby9zcmMvZ3VpL3F0LWRhZW1vbi9odG1sX3NvdXJjZS9zcmMvYXBwL3dhbGxldC93YWxsZXQuY29tcG9uZW50LnNjc3MiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IkFBQUE7RUFDRSxrQkFBa0I7RUFDbEIsb0JBQWE7RUFBYixhQUFhO0VBQ2IsNEJBQXNCO0VBQXRCLDZCQUFzQjtVQUF0QixzQkFBc0I7RUFDdEIsb0JBQW9CO0VBQ3BCLGdCQUFnQjtFQUNoQixXQUFXO0VBQ1gsWUFBWSxFQUFBOztBQUdkO0VBQ0Usb0JBQWE7RUFBYixhQUFhO0VBQ2IseUJBQW1CO1VBQW5CLG1CQUFtQjtFQUNuQix5QkFBOEI7VUFBOUIsOEJBQThCO0VBQzlCLG1CQUFjO1VBQWQsY0FBYztFQUNkLFlBQVksRUFBQTs7QUFMZDtJQVFJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLHlCQUFtQjtZQUFuQixtQkFBbUIsRUFBQTs7QUFUdkI7TUFZTSxvQkFBb0IsRUFBQTs7QUFaMUI7SUFpQkksaUJBQWlCO0lBQ2pCLGdCQUFnQjtJQUNoQix1QkFBdUI7SUFDdkIsZ0JBQWdCO0lBQ2hCLG1CQUFtQjtJQUNuQixnQkFBZ0I7SUFDaEIsbUJBQW1CLEVBQUE7O0FBdkJ2QjtJQTJCSSxvQkFBYTtJQUFiLGFBQWE7SUFDYix5QkFBbUI7WUFBbkIsbUJBQW1CO0lBQ25CLHVCQUF1QjtJQUN2QixZQUFZO0lBQ1osZUFBZTtJQUNmLGdCQUFnQjtJQUNoQixhQUFhO0lBQ2IsVUFBVSxFQUFBOztBQWxDZDtNQXFDTSxvQkFBb0I7TUFDcEIsYUFBYTtNQUNiLGNBQWMsRUFBQTs7QUF2Q3BCO1FBMENRLGlEQUEwRDtnQkFBMUQseUNBQTBELEVBQUE7O0FBMUNsRTtRQThDUSxpREFBMEQ7Z0JBQTFELHlDQUEwRCxFQUFBOztBQTlDbEU7UUFrRFEsOENBQXVEO2dCQUF2RCxzQ0FBdUQsRUFBQTs7QUFsRC9EO0lBd0RJLG9CQUFhO0lBQWIsYUFBYTtJQUNiLHlCQUFtQjtZQUFuQixtQkFBbUI7SUFDbkIsaUJBQWlCLEVBQUE7O0FBMURyQjtNQTZETSxlQUFlO01BQ2Ysb0JBQW9CO01BQ3BCLGFBQWE7TUFDYixjQUFjLEVBQUE7O0FBaEVwQjtRQW1FUSxpREFBMEQ7Z0JBQTFELHlDQUEwRCxFQUFBOztBQW5FbEU7UUF1RVEsOENBQXVEO2dCQUF2RCxzQ0FBdUQsRUFBQTs7QUFNL0Q7RUFDRSxvQkFBYTtFQUFiLGFBQWE7RUFDYix5QkFBbUI7VUFBbkIsbUJBQW1CO0VBQ25CLG1CQUFjO1VBQWQsY0FBYztFQUNkLGlCQUFpQjtFQUNqQixtQkFBbUIsRUFBQTs7QUFMckI7SUFRSSxlQUFlO0lBQ2YsbUJBQW1CO0lBQ25CLGFBQWE7SUFDYixjQUFjLEVBQUE7O0FBWGxCO01BY00sOENBQXVEO2NBQXZELHNDQUF1RCxFQUFBOztBQWQ3RDtRQWlCUSxhQUFhLEVBQUE7O0FBakJyQjtNQXNCTSw2REFBc0U7Y0FBdEUscURBQXNFLEVBQUE7O0FBSzVFO0VBQ0Usb0JBQWE7RUFBYixhQUFhO0VBQ2Isc0JBQXFCO1VBQXJCLHFCQUFxQjtFQUNyQix1QkFBMkI7VUFBM0IsMkJBQTJCO0VBQzNCLG1CQUFjO1VBQWQsY0FBYztFQUNkLGdCQUFnQixFQUFBOztBQUxsQjtJQVFJLGlCQUFpQjtJQUNqQixnQkFBZ0I7SUFDaEIsbUJBQW1CO0lBQ25CLG9CQUFvQixFQUFBOztBQVh4QjtJQWVJLGlCQUFpQjtJQUNqQixnQkFBZ0I7SUFDaEIsbUJBQW1CLEVBQUE7O0FBSXZCO0VBQ0Usb0JBQWE7RUFBYixhQUFhO0VBQ2IsNEJBQXNCO0VBQXRCLDZCQUFzQjtVQUF0QixzQkFBc0I7RUFDdEIsbUJBQWM7VUFBZCxjQUFjLEVBQUE7O0FBSGhCO0lBTUksb0JBQWE7SUFBYixhQUFhO0lBQ2IseUJBQThCO1lBQTlCLDhCQUE4QjtJQUM5QixtQkFBYztZQUFkLGNBQWMsRUFBQTs7QUFSbEI7TUFXTSxvQkFBYTtNQUFiLGFBQWE7TUFDYix5QkFBbUI7Y0FBbkIsbUJBQW1CO01BQ25CLHdCQUF1QjtjQUF2Qix1QkFBdUI7TUFDdkIsbUJBQWM7Y0FBZCxjQUFjO01BQ2QsZUFBZTtNQUNmLGVBQWU7TUFDZixZQUFZLEVBQUE7O0FBakJsQjtRQW9CUSxvQkFBb0I7UUFDcEIsYUFBYTtRQUNiLGNBQWMsRUFBQTs7QUF0QnRCO1VBeUJVLDhDQUF1RDtrQkFBdkQsc0NBQXVELEVBQUE7O0FBekJqRTtVQTZCVSxpREFBMEQ7a0JBQTFELHlDQUEwRCxFQUFBOztBQTdCcEU7VUFpQ1UsaURBQTBEO2tCQUExRCx5Q0FBMEQsRUFBQTs7QUFqQ3BFO1VBcUNVLG1EQUE0RDtrQkFBNUQsMkNBQTRELEVBQUE7O0FBckN0RTtVQXlDVSxpREFBMEQ7a0JBQTFELHlDQUEwRCxFQUFBOztBQXpDcEU7VUE2Q1UsaURBQTBEO2tCQUExRCx5Q0FBMEQsRUFBQTs7QUE3Q3BFO1FBa0RRLG9CQUFhO1FBQWIsYUFBYTtRQUNiLHlCQUFtQjtnQkFBbkIsbUJBQW1CO1FBQ25CLHdCQUF1QjtnQkFBdkIsdUJBQXVCO1FBQ3ZCLG1CQUFtQjtRQUNuQixlQUFlO1FBQ2YsZ0JBQWdCO1FBQ2hCLG1CQUFtQjtRQUNuQixpQkFBaUI7UUFDakIsaUJBQWlCO1FBQ2pCLGNBQWMsRUFBQTs7QUEzRHRCO1FBK0RRLDJDQUE0RCxFQUFBOztBQS9EcEU7UUFtRVEsb0JBQW9CLEVBQUE7O0FBbkU1QjtJQXlFSSxvQkFBYTtJQUFiLGFBQWE7SUFDYixhQUFhO0lBQ2IsbUJBQWM7WUFBZCxjQUFjO0lBQ2Qsa0JBQWtCO0lBQ2xCLG1CQUFtQixFQUFBIiwiZmlsZSI6InNyYy9hcHAvd2FsbGV0L3dhbGxldC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcbiAgcG9zaXRpb246IHJlbGF0aXZlO1xuICBkaXNwbGF5OiBmbGV4O1xuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xuICBwYWRkaW5nOiAwIDNyZW0gM3JlbTtcbiAgbWluLXdpZHRoOiA5NXJlbTtcbiAgd2lkdGg6IDEwMCU7XG4gIGhlaWdodDogMTAwJTtcbn1cblxuLmhlYWRlciB7XG4gIGRpc3BsYXk6IGZsZXg7XG4gIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG4gIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcbiAgZmxleDogMCAwIGF1dG87XG4gIGhlaWdodDogOHJlbTtcblxuICA+IGRpdiB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuXG4gICAgOm5vdCg6bGFzdC1jaGlsZCkge1xuICAgICAgbWFyZ2luLXJpZ2h0OiAzLjJyZW07XG4gICAgfVxuICB9XG5cbiAgaDMge1xuICAgIGZvbnQtc2l6ZTogMS43cmVtO1xuICAgIGZvbnQtd2VpZ2h0OiA2MDA7XG4gICAgdGV4dC1vdmVyZmxvdzogZWxsaXBzaXM7XG4gICAgb3ZlcmZsb3c6IGhpZGRlbjtcbiAgICB3aGl0ZS1zcGFjZTogbm93cmFwO1xuICAgIG1heC13aWR0aDogNTByZW07XG4gICAgbGluZS1oZWlnaHQ6IDIuN3JlbTtcbiAgfVxuXG4gIGJ1dHRvbiB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xuICAgIGJhY2tncm91bmQ6IHRyYW5zcGFyZW50O1xuICAgIGJvcmRlcjogbm9uZTtcbiAgICBjdXJzb3I6IHBvaW50ZXI7XG4gICAgZm9udC13ZWlnaHQ6IDQwMDtcbiAgICBvdXRsaW5lOiBub25lO1xuICAgIHBhZGRpbmc6IDA7XG5cbiAgICAuaWNvbiB7XG4gICAgICBtYXJnaW4tcmlnaHQ6IDEuMnJlbTtcbiAgICAgIHdpZHRoOiAxLjdyZW07XG4gICAgICBoZWlnaHQ6IDEuN3JlbTtcblxuICAgICAgJi5hY2NvdW50IHtcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9hY2NvdW50LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgIH1cblxuICAgICAgJi5kZXRhaWxzIHtcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9kZXRhaWxzLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgIH1cblxuICAgICAgJi5sb2NrIHtcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9sb2NrLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgIH1cbiAgICB9XG4gIH1cblxuICAuYWxpYXMge1xuICAgIGRpc3BsYXk6IGZsZXg7XG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICBmb250LXNpemU6IDEuM3JlbTtcblxuICAgIC5pY29uIHtcbiAgICAgIGN1cnNvcjogcG9pbnRlcjtcbiAgICAgIG1hcmdpbi1yaWdodDogMS4ycmVtO1xuICAgICAgd2lkdGg6IDEuN3JlbTtcbiAgICAgIGhlaWdodDogMS43cmVtO1xuXG4gICAgICAmLmVkaXQge1xuICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2RldGFpbHMuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xuICAgICAgfVxuXG4gICAgICAmLnRyYW5zZmVyIHtcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9zZW5kLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgIH1cbiAgICB9XG4gIH1cbn1cblxuLmFkZHJlc3Mge1xuICBkaXNwbGF5OiBmbGV4O1xuICBhbGlnbi1pdGVtczogY2VudGVyO1xuICBmbGV4OiAwIDAgYXV0bztcbiAgZm9udC1zaXplOiAxLjRyZW07XG4gIGxpbmUtaGVpZ2h0OiAxLjdyZW07XG5cbiAgLmljb24ge1xuICAgIGN1cnNvcjogcG9pbnRlcjtcbiAgICBtYXJnaW4tbGVmdDogMS4ycmVtO1xuICAgIHdpZHRoOiAxLjdyZW07XG4gICAgaGVpZ2h0OiAxLjdyZW07XG5cbiAgICAmLmNvcHkge1xuICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9jb3B5LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcblxuICAgICAgJjpob3ZlciB7XG4gICAgICAgIG9wYWNpdHk6IDAuNzU7XG4gICAgICB9XG4gICAgfVxuXG4gICAgJi5jb3BpZWQge1xuICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9jb21wbGV0ZS10ZXN0d2FsbGV0LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICB9XG4gIH1cbn1cblxuLmJhbGFuY2Uge1xuICBkaXNwbGF5OiBmbGV4O1xuICBhbGlnbi1pdGVtczogZmxleC1lbmQ7XG4gIGp1c3RpZnktY29udGVudDogZmxleC1zdGFydDtcbiAgZmxleDogMCAwIGF1dG87XG4gIG1hcmdpbjogMi42cmVtIDA7XG5cbiAgOmZpcnN0LWNoaWxkIHtcbiAgICBmb250LXNpemU6IDMuM3JlbTtcbiAgICBmb250LXdlaWdodDogNjAwO1xuICAgIGxpbmUtaGVpZ2h0OiAyLjRyZW07XG4gICAgbWFyZ2luLXJpZ2h0OiAzLjVyZW07XG4gIH1cblxuICA6bGFzdC1jaGlsZCB7XG4gICAgZm9udC1zaXplOiAxLjhyZW07XG4gICAgZm9udC13ZWlnaHQ6IDYwMDtcbiAgICBsaW5lLWhlaWdodDogMS4zcmVtO1xuICB9XG59XG5cbi50YWJzIHtcbiAgZGlzcGxheTogZmxleDtcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcbiAgZmxleDogMSAxIGF1dG87XG5cbiAgLnRhYnMtaGVhZGVyIHtcbiAgICBkaXNwbGF5OiBmbGV4O1xuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcbiAgICBmbGV4OiAwIDAgYXV0bztcblxuICAgIC50YWIge1xuICAgICAgZGlzcGxheTogZmxleDtcbiAgICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XG4gICAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcbiAgICAgIGZsZXg6IDEgMCBhdXRvO1xuICAgICAgY3Vyc29yOiBwb2ludGVyO1xuICAgICAgcGFkZGluZzogMCAxcmVtO1xuICAgICAgaGVpZ2h0OiA1cmVtO1xuXG4gICAgICAuaWNvbiB7XG4gICAgICAgIG1hcmdpbi1yaWdodDogMS4zcmVtO1xuICAgICAgICB3aWR0aDogMS43cmVtO1xuICAgICAgICBoZWlnaHQ6IDEuN3JlbTtcblxuICAgICAgICAmLnNlbmQge1xuICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvc2VuZC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICAgIH1cblxuICAgICAgICAmLnJlY2VpdmUge1xuICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvcmVjZWl2ZS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICAgIH1cblxuICAgICAgICAmLmhpc3Rvcnkge1xuICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvaGlzdG9yeS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XG4gICAgICAgIH1cblxuICAgICAgICAmLmNvbnRyYWN0cyB7XG4gICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9jb250cmFjdHMuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xuICAgICAgICB9XG5cbiAgICAgICAgJi5tZXNzYWdlcyB7XG4gICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9tZXNzYWdlLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgICAgfVxuXG4gICAgICAgICYuc3Rha2luZyB7XG4gICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9zdGFraW5nLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcbiAgICAgICAgfVxuICAgICAgfVxuXG4gICAgICAuaW5kaWNhdG9yIHtcbiAgICAgICAgZGlzcGxheTogZmxleDtcbiAgICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcbiAgICAgICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XG4gICAgICAgIGJvcmRlci1yYWRpdXM6IDFyZW07XG4gICAgICAgIGZvbnQtc2l6ZTogMXJlbTtcbiAgICAgICAgZm9udC13ZWlnaHQ6IDYwMDtcbiAgICAgICAgbWFyZ2luLWxlZnQ6IDEuM3JlbTtcbiAgICAgICAgcGFkZGluZzogMCAwLjVyZW07XG4gICAgICAgIG1pbi13aWR0aDogMS42cmVtO1xuICAgICAgICBoZWlnaHQ6IDEuNnJlbTtcbiAgICAgIH1cblxuICAgICAgJi5kaXNhYmxlZCB7XG4gICAgICAgIGN1cnNvcjogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9ub3QtYWxsb3dlZC5zdmcpLCBub3QtYWxsb3dlZDtcbiAgICAgIH1cblxuICAgICAgJjpub3QoOmxhc3QtY2hpbGQpIHtcbiAgICAgICAgbWFyZ2luLXJpZ2h0OiAwLjNyZW07XG4gICAgICB9XG4gICAgfVxuICB9XG5cbiAgLnRhYnMtY29udGVudCB7XG4gICAgZGlzcGxheTogZmxleDtcbiAgICBwYWRkaW5nOiAzcmVtO1xuICAgIGZsZXg6IDEgMSBhdXRvO1xuICAgIG92ZXJmbG93LXg6IGhpZGRlbjtcbiAgICBvdmVyZmxvdy15OiBvdmVybGF5O1xuICB9XG59XG4iXX0= */"

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
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
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






var WalletComponent = /** @class */ (function () {
    function WalletComponent(route, router, backend, variablesService, ngZone, translate, intToMoneyPipe) {
        this.route = route;
        this.router = router;
        this.backend = backend;
        this.variablesService = variablesService;
        this.ngZone = ngZone;
        this.translate = translate;
        this.intToMoneyPipe = intToMoneyPipe;
        this.copyAnimation = false;
        this.tabs = [
            {
                title: 'WALLET.TABS.HISTORY',
                icon: 'history',
                link: '/history',
                indicator: false,
                active: true
            },
            {
                title: 'WALLET.TABS.SEND',
                icon: 'send',
                link: '/send',
                indicator: false,
                active: false
            },
            {
                title: 'WALLET.TABS.RECEIVE',
                icon: 'receive',
                link: '/receive',
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
            /*{
              title: 'WALLET.TABS.MESSAGES',
              icon: 'messages',
              link: '/messages',
              indicator: 32,
              active: false
            },*/
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
        this.subRouting1 = this.route.params.subscribe(function (params) {
            _this.walletID = +params['id'];
            _this.variablesService.setCurrentWallet(_this.walletID);
            _this.scrolledContent.nativeElement.scrollTop = 0;
            clearTimeout(_this.copyAnimationTimeout);
            _this.copyAnimation = false;
        });
        this.subRouting2 = this.router.events.subscribe(function (val) {
            if (val instanceof _angular_router__WEBPACK_IMPORTED_MODULE_1__["RoutesRecognized"]) {
                if (val.state.root.firstChild && val.state.root.firstChild.firstChild) {
                    for (var i = 0; i < _this.tabs.length; i++) {
                        _this.tabs[i].active = (_this.tabs[i].link === '/' + val.state.root.firstChild.firstChild.url[0].path);
                    }
                }
            }
        });
        if (this.variablesService.currentWallet.alias.hasOwnProperty('name')) {
            this.variablesService.currentWallet.wakeAlias = false;
        }
        this.aliasSubscription = this.variablesService.getAliasChangedEvent.subscribe(function () {
            if (_this.variablesService.currentWallet.alias.hasOwnProperty('name')) {
                _this.variablesService.currentWallet.wakeAlias = false;
            }
        });
    };
    WalletComponent.prototype.changeTab = function (index) {
        var _this = this;
        if ((this.tabs[index].link === '/send' || this.tabs[index].link === '/contracts' || this.tabs[index].link === '/staking') && (this.variablesService.daemon_state !== 2 || !this.variablesService.currentWallet.loaded)) {
            return;
        }
        this.tabs.forEach(function (tab) {
            tab.active = false;
        });
        this.tabs[index].active = true;
        this.ngZone.run(function () {
            _this.scrolledContent.nativeElement.scrollTop = 0;
            _this.router.navigate(['wallet/' + _this.walletID + _this.tabs[index].link]);
        });
    };
    WalletComponent.prototype.copyAddress = function () {
        var _this = this;
        this.backend.setClipboard(this.variablesService.currentWallet.address);
        this.copyAnimation = true;
        this.copyAnimationTimeout = window.setTimeout(function () {
            _this.copyAnimation = false;
        }, 2000);
    };
    WalletComponent.prototype.getTooltip = function () {
        var _this = this;
        this.balanceTooltip = document.createElement('div');
        var available = document.createElement('span');
        available.setAttribute('class', 'available');
        available.innerHTML = this.translate.instant('WALLET.AVAILABLE_BALANCE', { available: this.intToMoneyPipe.transform(this.variablesService.currentWallet.unlocked_balance), currency: this.variablesService.defaultCurrency });
        this.balanceTooltip.appendChild(available);
        var locked = document.createElement('span');
        locked.setAttribute('class', 'locked');
        locked.innerHTML = this.translate.instant('WALLET.LOCKED_BALANCE', { locked: this.intToMoneyPipe.transform(this.variablesService.currentWallet.balance.minus(this.variablesService.currentWallet.unlocked_balance)), currency: this.variablesService.defaultCurrency });
        this.balanceTooltip.appendChild(locked);
        var link = document.createElement('span');
        link.setAttribute('class', 'link');
        link.innerHTML = this.translate.instant('WALLET.LOCKED_BALANCE_LINK');
        link.addEventListener('click', function () {
            _this.openInBrowser('docs.zano.org/docs/locked-balance');
        });
        this.balanceTooltip.appendChild(link);
        return this.balanceTooltip;
    };
    WalletComponent.prototype.onHideTooltip = function () {
        this.balanceTooltip = null;
    };
    WalletComponent.prototype.openInBrowser = function (link) {
        this.backend.openUrlInBrowser(link);
    };
    WalletComponent.prototype.ngOnDestroy = function () {
        this.subRouting1.unsubscribe();
        this.subRouting2.unsubscribe();
        this.aliasSubscription.unsubscribe();
        clearTimeout(this.copyAnimationTimeout);
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["ViewChild"])('scrolledContent'),
        __metadata("design:type", _angular_core__WEBPACK_IMPORTED_MODULE_0__["ElementRef"])
    ], WalletComponent.prototype, "scrolledContent", void 0);
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
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_4__["TranslateService"],
            _helpers_pipes_int_to_money_pipe__WEBPACK_IMPORTED_MODULE_5__["IntToMoneyPipe"]])
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

module.exports = __webpack_require__(/*! /Users/apple/Documents/zano/src/gui/qt-daemon/html_source/src/main.ts */"./src/main.ts");


/***/ })

},[[0,"runtime","vendor"]]]);
//# sourceMappingURL=main.js.map