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

module.exports = "<div class=\"modal\">\r\n  <div class=\"content\">\r\n    <i class=\"icon\" [class.error]=\"type === 'error'\" [class.success]=\"type === 'success'\" [class.info]=\"type === 'info'\"></i>\r\n    <div class=\"message-container\">\r\n      <span class=\"title\">{{title}}</span>\r\n      <span class=\"message\" [innerHTML]=\"message\"></span>\r\n    </div>\r\n  </div>\r\n  <button type=\"button\" class=\"action-button\" (click)=\"onClose()\" #btn>{{ 'MODALS.OK' | translate }}</button>\r\n  <button type=\"button\" class=\"close-button\" (click)=\"onClose()\"><i class=\"icon close\"></i></button>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/_helpers/directives/modal-container/modal-container.component.scss":
/*!************************************************************************************!*\
  !*** ./src/app/_helpers/directives/modal-container/modal-container.component.scss ***!
  \************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: fixed;\n  top: 0;\n  bottom: 0;\n  left: 0;\n  right: 0;\n  display: flex;\n  align-items: center;\n  justify-content: center;\n  background: rgba(255, 255, 255, 0.25); }\n\n.modal {\n  position: relative;\n  display: flex;\n  flex-direction: column;\n  background-position: center;\n  background-size: 200%;\n  padding: 2rem;\n  width: 34rem; }\n\n.modal .content {\n    display: flex;\n    margin: 1.2rem 0; }\n\n.modal .content .icon {\n      flex: 0 0 auto;\n      width: 4.4rem;\n      height: 4.4rem; }\n\n.modal .content .icon.error {\n        -webkit-mask: url('modal-alert.svg') no-repeat center;\n                mask: url('modal-alert.svg') no-repeat center; }\n\n.modal .content .icon.success {\n        -webkit-mask: url('modal-success.svg') no-repeat center;\n                mask: url('modal-success.svg') no-repeat center; }\n\n.modal .content .icon.info {\n        -webkit-mask: url('modal-info.svg') no-repeat center;\n                mask: url('modal-info.svg') no-repeat center; }\n\n.modal .content .message-container {\n      display: flex;\n      flex-direction: column;\n      align-items: flex-start;\n      justify-content: center;\n      margin-left: 2rem; }\n\n.modal .content .message-container .title {\n        font-size: 1.8rem;\n        font-weight: 600;\n        line-height: 2.2rem; }\n\n.modal .content .message-container .message {\n        font-size: 1.3rem;\n        line-height: 1.8rem;\n        margin-top: 0.4rem; }\n\n.modal .action-button {\n    margin: 1.2rem auto 0.6rem;\n    width: 10rem;\n    height: 2.4rem; }\n\n.modal .close-button {\n    position: absolute;\n    top: 0;\n    right: 0;\n    display: flex;\n    align-items: center;\n    justify-content: center;\n    background: transparent;\n    margin: 0;\n    padding: 0;\n    width: 2.4rem;\n    height: 2.4rem; }\n\n.modal .close-button .icon {\n      -webkit-mask: url('close.svg') no-repeat center;\n              mask: url('close.svg') no-repeat center;\n      width: 2.4rem;\n      height: 2.4rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9faGVscGVycy9kaXJlY3RpdmVzL21vZGFsLWNvbnRhaW5lci9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcX2hlbHBlcnNcXGRpcmVjdGl2ZXNcXG1vZGFsLWNvbnRhaW5lclxcbW9kYWwtY29udGFpbmVyLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZUFBZTtFQUNmLE1BQU07RUFDTixTQUFTO0VBQ1QsT0FBTztFQUNQLFFBQVE7RUFDUixhQUFhO0VBQ2IsbUJBQW1CO0VBQ25CLHVCQUF1QjtFQUN2QixxQ0FBcUMsRUFBQTs7QUFFdkM7RUFDRSxrQkFBa0I7RUFDbEIsYUFBYTtFQUNiLHNCQUFzQjtFQUN0QiwyQkFBMkI7RUFDM0IscUJBQXFCO0VBQ3JCLGFBQWE7RUFDYixZQUFZLEVBQUE7O0FBUGQ7SUFVSSxhQUFhO0lBQ2IsZ0JBQWdCLEVBQUE7O0FBWHBCO01BY00sY0FBYztNQUNkLGFBQWE7TUFDYixjQUFjLEVBQUE7O0FBaEJwQjtRQW1CUSxxREFBNkQ7Z0JBQTdELDZDQUE2RCxFQUFBOztBQW5CckU7UUF1QlEsdURBQStEO2dCQUEvRCwrQ0FBK0QsRUFBQTs7QUF2QnZFO1FBMkJRLG9EQUE0RDtnQkFBNUQsNENBQTRELEVBQUE7O0FBM0JwRTtNQWdDTSxhQUFhO01BQ2Isc0JBQXNCO01BQ3RCLHVCQUF1QjtNQUN2Qix1QkFBdUI7TUFDdkIsaUJBQWlCLEVBQUE7O0FBcEN2QjtRQXVDUSxpQkFBaUI7UUFDakIsZ0JBQWdCO1FBQ2hCLG1CQUFtQixFQUFBOztBQXpDM0I7UUE2Q1EsaUJBQWlCO1FBQ2pCLG1CQUFtQjtRQUNuQixrQkFBa0IsRUFBQTs7QUEvQzFCO0lBcURJLDBCQUEwQjtJQUMxQixZQUFZO0lBQ1osY0FBYyxFQUFBOztBQXZEbEI7SUEyREksa0JBQWtCO0lBQ2xCLE1BQU07SUFDTixRQUFRO0lBQ1IsYUFBYTtJQUNiLG1CQUFtQjtJQUNuQix1QkFBdUI7SUFDdkIsdUJBQXVCO0lBQ3ZCLFNBQVM7SUFDVCxVQUFVO0lBQ1YsYUFBYTtJQUNiLGNBQWMsRUFBQTs7QUFyRWxCO01Bd0VNLCtDQUF1RDtjQUF2RCx1Q0FBdUQ7TUFDdkQsYUFBYTtNQUNiLGNBQWMsRUFBQSIsImZpbGUiOiJhcHAvX2hlbHBlcnMvZGlyZWN0aXZlcy9tb2RhbC1jb250YWluZXIvbW9kYWwtY29udGFpbmVyLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xyXG4gIHBvc2l0aW9uOiBmaXhlZDtcclxuICB0b3A6IDA7XHJcbiAgYm90dG9tOiAwO1xyXG4gIGxlZnQ6IDA7XHJcbiAgcmlnaHQ6IDA7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gIGp1c3RpZnktY29udGVudDogY2VudGVyO1xyXG4gIGJhY2tncm91bmQ6IHJnYmEoMjU1LCAyNTUsIDI1NSwgMC4yNSk7XHJcbn1cclxuLm1vZGFsIHtcclxuICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gIGJhY2tncm91bmQtcG9zaXRpb246IGNlbnRlcjtcclxuICBiYWNrZ3JvdW5kLXNpemU6IDIwMCU7XHJcbiAgcGFkZGluZzogMnJlbTtcclxuICB3aWR0aDogMzRyZW07XHJcblxyXG4gIC5jb250ZW50IHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBtYXJnaW46IDEuMnJlbSAwO1xyXG5cclxuICAgIC5pY29uIHtcclxuICAgICAgZmxleDogMCAwIGF1dG87XHJcbiAgICAgIHdpZHRoOiA0LjRyZW07XHJcbiAgICAgIGhlaWdodDogNC40cmVtO1xyXG5cclxuICAgICAgJi5lcnJvciB7XHJcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL21vZGFsLWFsZXJ0LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi5zdWNjZXNzIHtcclxuICAgICAgICBtYXNrOiB1cmwofnNyYy9hc3NldHMvaWNvbnMvbW9kYWwtc3VjY2Vzcy5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYuaW5mbyB7XHJcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL21vZGFsLWluZm8uc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICB9XHJcbiAgICB9XHJcblxyXG4gICAgLm1lc3NhZ2UtY29udGFpbmVyIHtcclxuICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgICAgYWxpZ24taXRlbXM6IGZsZXgtc3RhcnQ7XHJcbiAgICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xyXG4gICAgICBtYXJnaW4tbGVmdDogMnJlbTtcclxuXHJcbiAgICAgIC50aXRsZSB7XHJcbiAgICAgICAgZm9udC1zaXplOiAxLjhyZW07XHJcbiAgICAgICAgZm9udC13ZWlnaHQ6IDYwMDtcclxuICAgICAgICBsaW5lLWhlaWdodDogMi4ycmVtO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAubWVzc2FnZSB7XHJcbiAgICAgICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICAgICAgbGluZS1oZWlnaHQ6IDEuOHJlbTtcclxuICAgICAgICBtYXJnaW4tdG9wOiAwLjRyZW07XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcblxyXG4gIC5hY3Rpb24tYnV0dG9uIHtcclxuICAgIG1hcmdpbjogMS4ycmVtIGF1dG8gMC42cmVtO1xyXG4gICAgd2lkdGg6IDEwcmVtO1xyXG4gICAgaGVpZ2h0OiAyLjRyZW07XHJcbiAgfVxyXG5cclxuICAuY2xvc2UtYnV0dG9uIHtcclxuICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuICAgIHRvcDogMDtcclxuICAgIHJpZ2h0OiAwO1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgIGJhY2tncm91bmQ6IHRyYW5zcGFyZW50O1xyXG4gICAgbWFyZ2luOiAwO1xyXG4gICAgcGFkZGluZzogMDtcclxuICAgIHdpZHRoOiAyLjRyZW07XHJcbiAgICBoZWlnaHQ6IDIuNHJlbTtcclxuXHJcbiAgICAuaWNvbiB7XHJcbiAgICAgIG1hc2s6IHVybCh+c3JjL2Fzc2V0cy9pY29ucy9jbG9zZS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIHdpZHRoOiAyLjRyZW07XHJcbiAgICAgIGhlaWdodDogMi40cmVtO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

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

module.exports = "<div class=\"progress-bar-container\">\r\n  <div class=\"progress-bar\">\r\n    <div class=\"progress-bar-full\" [style.width]=\"width\"></div>\r\n  </div>\r\n  <div class=\"progress-labels\">\r\n    <span *ngFor=\"let label of labels\">\r\n      {{ label | translate }}\r\n    </span>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/_helpers/directives/progress-container/progress-container.component.scss":
/*!******************************************************************************************!*\
  !*** ./src/app/_helpers/directives/progress-container/progress-container.component.scss ***!
  \******************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".progress-bar-container {\n  position: absolute;\n  bottom: 0;\n  left: 0;\n  padding: 0 3rem;\n  width: 100%;\n  height: 3rem; }\n  .progress-bar-container .progress-bar {\n    position: absolute;\n    top: -0.7rem;\n    left: 0;\n    margin: 0 3rem;\n    width: calc(100% - 6rem);\n    height: 0.7rem; }\n  .progress-bar-container .progress-bar .progress-bar-full {\n      height: 0.7rem; }\n  .progress-bar-container .progress-labels {\n    display: flex;\n    align-items: center;\n    justify-content: space-between;\n    font-size: 1.2rem;\n    height: 100%; }\n  .progress-bar-container .progress-labels span {\n      flex: 1 0 0;\n      text-align: center; }\n  .progress-bar-container .progress-labels span:first-child {\n        text-align: left; }\n  .progress-bar-container .progress-labels span:last-child {\n        text-align: right; }\n  .progress-bar-container .progress-time {\n    position: absolute;\n    top: -3rem;\n    left: 50%;\n    transform: translateX(-50%);\n    font-size: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9faGVscGVycy9kaXJlY3RpdmVzL3Byb2dyZXNzLWNvbnRhaW5lci9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcX2hlbHBlcnNcXGRpcmVjdGl2ZXNcXHByb2dyZXNzLWNvbnRhaW5lclxccHJvZ3Jlc3MtY29udGFpbmVyLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCO0VBQ2xCLFNBQVM7RUFDVCxPQUFPO0VBQ1AsZUFBZTtFQUNmLFdBQVc7RUFDWCxZQUFZLEVBQUE7RUFOZDtJQVNJLGtCQUFrQjtJQUNsQixZQUFZO0lBQ1osT0FBTztJQUNQLGNBQWM7SUFDZCx3QkFBd0I7SUFDeEIsY0FBYyxFQUFBO0VBZGxCO01BaUJNLGNBQWMsRUFBQTtFQWpCcEI7SUFzQkksYUFBYTtJQUNiLG1CQUFtQjtJQUNuQiw4QkFBOEI7SUFDOUIsaUJBQWlCO0lBQ2pCLFlBQVksRUFBQTtFQTFCaEI7TUE2Qk0sV0FBVztNQUNYLGtCQUFrQixFQUFBO0VBOUJ4QjtRQWlDUSxnQkFBZ0IsRUFBQTtFQWpDeEI7UUFxQ1EsaUJBQWlCLEVBQUE7RUFyQ3pCO0lBMkNJLGtCQUFrQjtJQUNsQixVQUFVO0lBQ1YsU0FBUztJQUNULDJCQUEyQjtJQUMzQixpQkFBaUIsRUFBQSIsImZpbGUiOiJhcHAvX2hlbHBlcnMvZGlyZWN0aXZlcy9wcm9ncmVzcy1jb250YWluZXIvcHJvZ3Jlc3MtY29udGFpbmVyLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLnByb2dyZXNzLWJhci1jb250YWluZXIge1xyXG4gIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuICBib3R0b206IDA7XHJcbiAgbGVmdDogMDtcclxuICBwYWRkaW5nOiAwIDNyZW07XHJcbiAgd2lkdGg6IDEwMCU7XHJcbiAgaGVpZ2h0OiAzcmVtO1xyXG5cclxuICAucHJvZ3Jlc3MtYmFyIHtcclxuICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuICAgIHRvcDogLTAuN3JlbTtcclxuICAgIGxlZnQ6IDA7XHJcbiAgICBtYXJnaW46IDAgM3JlbTtcclxuICAgIHdpZHRoOiBjYWxjKDEwMCUgLSA2cmVtKTtcclxuICAgIGhlaWdodDogMC43cmVtO1xyXG5cclxuICAgIC5wcm9ncmVzcy1iYXItZnVsbCB7XHJcbiAgICAgIGhlaWdodDogMC43cmVtO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnByb2dyZXNzLWxhYmVscyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICAgIGZvbnQtc2l6ZTogMS4ycmVtO1xyXG4gICAgaGVpZ2h0OiAxMDAlO1xyXG5cclxuICAgIHNwYW4ge1xyXG4gICAgICBmbGV4OiAxIDAgMDtcclxuICAgICAgdGV4dC1hbGlnbjogY2VudGVyO1xyXG5cclxuICAgICAgJjpmaXJzdC1jaGlsZCB7XHJcbiAgICAgICAgdGV4dC1hbGlnbjogbGVmdDtcclxuICAgICAgfVxyXG5cclxuICAgICAgJjpsYXN0LWNoaWxkIHtcclxuICAgICAgICB0ZXh0LWFsaWduOiByaWdodDtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnByb2dyZXNzLXRpbWUge1xyXG4gICAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gICAgdG9wOiAtM3JlbTtcclxuICAgIGxlZnQ6IDUwJTtcclxuICAgIHRyYW5zZm9ybTogdHJhbnNsYXRlWCgtNTAlKTtcclxuICAgIGZvbnQtc2l6ZTogMS4ycmVtO1xyXG4gIH1cclxufVxyXG4iXX0= */"

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

module.exports = "<div class=\"switch\" (click)=\"toggleStaking(); $event.stopPropagation()\">\r\n  <span class=\"option\" *ngIf=\"staking\">{{ 'STAKING.SWITCH.ON' | translate }}</span>\r\n  <span class=\"circle\" [class.on]=\"staking\" [class.off]=\"!staking\"></span>\r\n  <span class=\"option\" *ngIf=\"!staking\">{{ 'STAKING.SWITCH.OFF' | translate }}</span>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/_helpers/directives/staking-switch/staking-switch.component.scss":
/*!**********************************************************************************!*\
  !*** ./src/app/_helpers/directives/staking-switch/staking-switch.component.scss ***!
  \**********************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".switch {\n  display: flex;\n  align-items: center;\n  justify-content: space-between;\n  border-radius: 1rem;\n  cursor: pointer;\n  font-size: 1rem;\n  padding: 0.5rem;\n  width: 5rem;\n  height: 2rem; }\n  .switch .circle {\n    border-radius: 1rem;\n    width: 1.2rem;\n    height: 1.2rem; }\n  .switch .option {\n    margin: 0 0.2rem;\n    line-height: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9faGVscGVycy9kaXJlY3RpdmVzL3N0YWtpbmctc3dpdGNoL0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxfaGVscGVyc1xcZGlyZWN0aXZlc1xcc3Rha2luZy1zd2l0Y2hcXHN0YWtpbmctc3dpdGNoLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsYUFBYTtFQUNiLG1CQUFtQjtFQUNuQiw4QkFBOEI7RUFDOUIsbUJBQW1CO0VBQ25CLGVBQWU7RUFDZixlQUFlO0VBQ2YsZUFBZTtFQUNmLFdBQVc7RUFDWCxZQUFZLEVBQUE7RUFUZDtJQVlJLG1CQUFtQjtJQUNuQixhQUFhO0lBQ2IsY0FBYyxFQUFBO0VBZGxCO0lBa0JJLGdCQUFnQjtJQUNoQixtQkFBbUIsRUFBQSIsImZpbGUiOiJhcHAvX2hlbHBlcnMvZGlyZWN0aXZlcy9zdGFraW5nLXN3aXRjaC9zdGFraW5nLXN3aXRjaC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIi5zd2l0Y2gge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgYm9yZGVyLXJhZGl1czogMXJlbTtcclxuICBjdXJzb3I6IHBvaW50ZXI7XHJcbiAgZm9udC1zaXplOiAxcmVtO1xyXG4gIHBhZGRpbmc6IDAuNXJlbTtcclxuICB3aWR0aDogNXJlbTtcclxuICBoZWlnaHQ6IDJyZW07XHJcblxyXG4gIC5jaXJjbGUge1xyXG4gICAgYm9yZGVyLXJhZGl1czogMXJlbTtcclxuICAgIHdpZHRoOiAxLjJyZW07XHJcbiAgICBoZWlnaHQ6IDEuMnJlbTtcclxuICB9XHJcblxyXG4gIC5vcHRpb24ge1xyXG4gICAgbWFyZ2luOiAwIDAuMnJlbTtcclxuICAgIGxpbmUtaGVpZ2h0OiAxLjJyZW07XHJcbiAgfVxyXG59XHJcbiJdfQ== */"

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

module.exports = "<div class=\"table\">\r\n  <div class=\"row\">\r\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.ID' | translate }}</span>\r\n    <span class=\"cell key-value\" [style.flex-basis]=\"sizes[1] + 'px'\" (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, transaction.tx_hash)\" (click)=\"openInBrowser(transaction.tx_hash)\">{{transaction.tx_hash}}</span>\r\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[2] + 'px'\">{{ 'HISTORY.DETAILS.SIZE' | translate }}</span>\r\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[3] + 'px'\">{{ 'HISTORY.DETAILS.SIZE_VALUE' | translate : {value: transaction.tx_blob_size} }}</span>\r\n  </div>\r\n  <div class=\"row\">\r\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.HEIGHT' | translate }}</span>\r\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[1] + 'px'\">{{transaction.height}}</span>\r\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[2] + 'px'\">{{ 'HISTORY.DETAILS.CONFIRMATION' | translate }}</span>\r\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[3] + 'px'\">{{transaction.height === 0 ? 0 : variablesService.height_app - transaction.height}}</span>\r\n  </div>\r\n  <div class=\"row\">\r\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.INPUTS' | translate }}</span>\r\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[1] + 'px'\" tooltip=\"{{inputs.join(', ')}}\" placement=\"top\" tooltipClass=\"table-tooltip table-tooltip-dimensions\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{inputs.join(', ')}}</span>\r\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[2] + 'px'\">{{ 'HISTORY.DETAILS.OUTPUTS' | translate }}</span>\r\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[3] + 'px'\" tooltip=\"{{outputs.join(', ')}}\" placement=\"top\" tooltipClass=\"table-tooltip table-tooltip-dimensions\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{outputs.join(', ')}}</span>\r\n  </div>\r\n  <div class=\"row\">\r\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.PAYMENT_ID' | translate }}</span>\r\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[1] + sizes[2] + sizes[3] + 'px'\"\r\n          tooltip=\"{{transaction.payment_id}}\" placement=\"top\" tooltipClass=\"table-tooltip comment-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">\r\n      {{transaction.payment_id}}\r\n    </span>\r\n  </div>\r\n  <div class=\"row\">\r\n    <span class=\"cell label\" [style.flex-basis]=\"sizes[0] + 'px'\">{{ 'HISTORY.DETAILS.COMMENT' | translate }}</span>\r\n    <span class=\"cell value\" [style.flex-basis]=\"sizes[1] + sizes[2] + sizes[3] + 'px'\"\r\n          tooltip=\"{{transaction.comment}}\" placement=\"top\" tooltipClass=\"table-tooltip comment-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\"\r\n          (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, transaction.comment)\">\r\n      {{transaction.comment}}\r\n    </span>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/_helpers/directives/transaction-details/transaction-details.component.scss":
/*!********************************************************************************************!*\
  !*** ./src/app/_helpers/directives/transaction-details/transaction-details.component.scss ***!
  \********************************************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: absolute;\n  top: 0;\n  left: 0;\n  width: 100%; }\n\n.table {\n  border-top: 0.2rem solid #ebebeb;\n  margin: 0 3rem;\n  padding: 0.5rem 0; }\n\n.table .row {\n    display: flex;\n    justify-content: flex-start;\n    align-items: center;\n    border-top: none;\n    line-height: 3rem;\n    margin: 0 -3rem;\n    width: 100%;\n    height: 3rem; }\n\n.table .row .cell {\n      flex-shrink: 0;\n      flex-grow: 0;\n      padding: 0 1rem;\n      overflow: hidden;\n      text-overflow: ellipsis; }\n\n.table .row .cell:first-child {\n        padding-left: 3rem; }\n\n.table .row .cell:last-child {\n        padding-right: 3rem; }\n\n.table .row .cell.key-value {\n        cursor: pointer; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9faGVscGVycy9kaXJlY3RpdmVzL3RyYW5zYWN0aW9uLWRldGFpbHMvRDpcXFByb2plY3RzX25vd1xcWkFOT1xcemFub1xcc3JjXFxndWlcXHF0LWRhZW1vblxcaHRtbF9zb3VyY2VcXHNyYy9hcHBcXF9oZWxwZXJzXFxkaXJlY3RpdmVzXFx0cmFuc2FjdGlvbi1kZXRhaWxzXFx0cmFuc2FjdGlvbi1kZXRhaWxzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCO0VBQ2xCLE1BQU07RUFDTixPQUFPO0VBQ1AsV0FBVyxFQUFBOztBQUdiO0VBQ0UsZ0NBQWdDO0VBQ2hDLGNBQWM7RUFDZCxpQkFBaUIsRUFBQTs7QUFIbkI7SUFNSSxhQUFhO0lBQ2IsMkJBQTJCO0lBQzNCLG1CQUFtQjtJQUNuQixnQkFBZ0I7SUFDaEIsaUJBQWlCO0lBQ2pCLGVBQWU7SUFDZixXQUFXO0lBQ1gsWUFBWSxFQUFBOztBQWJoQjtNQWdCTSxjQUFjO01BQ2QsWUFBWTtNQUNaLGVBQWU7TUFDZixnQkFBZ0I7TUFDaEIsdUJBQXVCLEVBQUE7O0FBcEI3QjtRQXVCUSxrQkFBa0IsRUFBQTs7QUF2QjFCO1FBMkJRLG1CQUFtQixFQUFBOztBQTNCM0I7UUErQlEsZUFBZSxFQUFBIiwiZmlsZSI6ImFwcC9faGVscGVycy9kaXJlY3RpdmVzL3RyYW5zYWN0aW9uLWRldGFpbHMvdHJhbnNhY3Rpb24tZGV0YWlscy5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBwb3NpdGlvbjogYWJzb2x1dGU7XHJcbiAgdG9wOiAwO1xyXG4gIGxlZnQ6IDA7XHJcbiAgd2lkdGg6IDEwMCU7XHJcbn1cclxuXHJcbi50YWJsZSB7XHJcbiAgYm9yZGVyLXRvcDogMC4ycmVtIHNvbGlkICNlYmViZWI7XHJcbiAgbWFyZ2luOiAwIDNyZW07XHJcbiAgcGFkZGluZzogMC41cmVtIDA7XHJcblxyXG4gIC5yb3cge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGp1c3RpZnktY29udGVudDogZmxleC1zdGFydDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBib3JkZXItdG9wOiBub25lO1xyXG4gICAgbGluZS1oZWlnaHQ6IDNyZW07XHJcbiAgICBtYXJnaW46IDAgLTNyZW07XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIGhlaWdodDogM3JlbTtcclxuXHJcbiAgICAuY2VsbCB7XHJcbiAgICAgIGZsZXgtc2hyaW5rOiAwO1xyXG4gICAgICBmbGV4LWdyb3c6IDA7XHJcbiAgICAgIHBhZGRpbmc6IDAgMXJlbTtcclxuICAgICAgb3ZlcmZsb3c6IGhpZGRlbjtcclxuICAgICAgdGV4dC1vdmVyZmxvdzogZWxsaXBzaXM7XHJcblxyXG4gICAgICAmOmZpcnN0LWNoaWxkIHtcclxuICAgICAgICBwYWRkaW5nLWxlZnQ6IDNyZW07XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICY6bGFzdC1jaGlsZCB7XHJcbiAgICAgICAgcGFkZGluZy1yaWdodDogM3JlbTtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi5rZXktdmFsdWUge1xyXG4gICAgICAgIGN1cnNvcjogcG9pbnRlcjtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

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
    BackendService.prototype.setMasterPassword = function (pass, callback) {
        this.runCommand('set_master_password', pass, callback);
    };
    BackendService.prototype.checkMasterPassword = function (pass, callback) {
        this.runCommand('check_master_password', pass, callback);
    };
    BackendService.prototype.storeSecureAppData = function (callback) {
        var _this = this;
        var data;
        var wallets = [];
        var contacts = [];
        this.variablesService.wallets.forEach(function (wallet) {
            wallets.push({ name: wallet.name, pass: wallet.pass, path: wallet.path, staking: wallet.staking });
        });
        this.variablesService.contacts.forEach(function (contact) {
            contacts.push({ name: contact.name, address: contact.address, notes: contact.notes });
        });
        data = { wallets: wallets, contacts: contacts };
        this.backendObject['store_secure_app_data'](JSON.stringify(data), this.variablesService.appPass, function (dataStore) {
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
    BackendService.prototype.storeFile = function (path, buff) {
        this.backendObject['store_to_file'](path, (typeof buff === 'string' ? buff : JSON.stringify(buff)));
    };
    BackendService.prototype.loadFile = function (path, callback) {
        this.runCommand('load_from_file', path, callback);
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
    BackendService.prototype.getContactAlias = function () {
        var _this = this;
        if (this.variablesService.contacts.length && this.variablesService.daemon_state === 2) {
            this.variablesService.contacts.map(function (contact) {
                _this.getAliasByAddress(contact.address, function (status, data) {
                    if (status) {
                        if (data.alias) {
                            contact.alias = '@' + data.alias;
                        }
                    }
                    else {
                        contact.alias = null;
                    }
                });
            });
        }
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
        this.dataIsLoaded = false;
        this.contacts = [];
        this.newContact = { name: null, address: null, notes: null };
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

/***/ "./src/app/add-contacts/add-contacts.component.html":
/*!**********************************************************!*\
  !*** ./src/app/add-contacts/add-contacts.component.html ***!
  \**********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content scrolled-content\">\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span [routerLink]=\"['/contacts']\">{{ 'CONTACTS.TITLE' | translate }}</span>\n      <span>{{ 'CONTACTS.ADD' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <form class=\"form-add\" [formGroup]=\"addContactForm\" (ngSubmit)=\"add()\">\n\n    <div class=\"input-block input-block-name\">\n      <label for=\"add-name\">{{ 'CONTACTS.FORM.NAME' | translate }}</label>\n      <input type=\"text\" id=\"add-name\" formControlName=\"name\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      <div class=\"error-block\" *ngIf=\"addContactForm.controls['name'].invalid && (addContactForm.controls['name'].dirty || addContactForm.controls['name'].touched)\">\n        <div *ngIf=\"addContactForm.controls['name'].errors['pattern']\">\n          {{ 'CONTACTS.FORM_ERRORS.NAME_WRONG' | translate }}\n        </div>\n        <div *ngIf=\"addContactForm.get('name').value.length <= 4 || addContactForm.get('name').value.length > 25\">\n          {{ 'CONTACTS.FORM_ERRORS.NAME_LENGTH' | translate }}\n        </div>\n        <div *ngIf=\"addContactForm.controls['name'].errors['required']\">\n          {{ 'CONTACTS.FORM_ERRORS.NAME_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"addContactForm.controls['name'].errors['dublicated']\">\n          {{ 'CONTACTS.FORM_ERRORS.NAME_DUBLICATED' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"input-block input-block-alias\">\n      <label for=\"address\">{{ 'CONTACTS.FORM.ADDRESS' | translate }}</label>\n  \n      <input type=\"text\" id=\"address\" formControlName=\"address\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n  \n      <div class=\"error-block\" *ngIf=\"addContactForm.controls['address'].invalid && (addContactForm.controls['address'].dirty || addContactForm.controls['address'].touched)\">\n        <div *ngIf=\"addContactForm.controls['address'].errors['required']\">\n          {{ 'CONTACTS.FORM_ERRORS.ADDRESS_REQUIRED' | translate }}\n        </div>\n        <div *ngIf=\"addContactForm.controls['address'].errors['address_not_valid']\">\n          {{ 'CONTACTS.FORM_ERRORS.ADDRESS_NOT_VALID' | translate }}\n        </div>\n        <div *ngIf=\"addContactForm.controls['address'].errors['dublicated']\">\n          {{ 'CONTACTS.FORM_ERRORS.ADDRESS_DUBLICATED' | translate }}\n        </div>\n      </div>\n    </div>\n\n    <div class=\"input-block input-block-notes\">\n      <label for=\"notes\">{{ 'CONTACTS.FORM.NOTES' | translate }}</label>\n  \n      <input type=\"text\" id=\"notes\" formControlName=\"notes\" (contextmenu)=\"variablesService.onContextMenu($event)\">\n      \n      <div class=\"error-block\" *ngIf=\"addContactForm.controls['notes'].invalid\">\n        <div *ngIf=\"addContactForm.controls['notes'].errors['maxLength']\">\n          {{ 'CONTACTS.FORM_ERRORS.MAX_LENGTH' | translate }}\n        </div>\n      </div>\n    </div>\n   \n    <button type=\"submit\" class=\"blue-button\" [disabled]=\"!addContactForm.valid\">{{ 'CONTACTS.BUTTON.ADD_EDIT' | translate }}</button>\n  \n    <app-send-modal *ngIf=\"isModalDialogVisible\" [form]=\"addContactForm\" (confirmed)=\"confirmed($event)\"></app-send-modal>\n  \n  </form>\n</div>\n"

/***/ }),

/***/ "./src/app/add-contacts/add-contacts.component.scss":
/*!**********************************************************!*\
  !*** ./src/app/add-contacts/add-contacts.component.scss ***!
  \**********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-add {\n  margin-top: 3rem; }\n  .form-add .input-block-name {\n    width: 50%; }\n  .form-add button {\n    margin-top: 3rem;\n    width: 100%;\n    max-width: 18rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9hZGQtY29udGFjdHMvRDpcXFByb2plY3RzX25vd1xcWkFOT1xcemFub1xcc3JjXFxndWlcXHF0LWRhZW1vblxcaHRtbF9zb3VyY2VcXHNyYy9hcHBcXGFkZC1jb250YWN0c1xcYWRkLWNvbnRhY3RzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZ0JBQWdCLEVBQUE7RUFEbEI7SUFJSSxVQUFVLEVBQUE7RUFKZDtJQVFJLGdCQUFnQjtJQUNoQixXQUFXO0lBQ1gsZ0JBQWdCLEVBQUEiLCJmaWxlIjoiYXBwL2FkZC1jb250YWN0cy9hZGQtY29udGFjdHMuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyIuZm9ybS1hZGQge1xyXG4gIG1hcmdpbi10b3A6IDNyZW07XHJcblxyXG4gIC5pbnB1dC1ibG9jay1uYW1lIHtcclxuICAgIHdpZHRoOiA1MCU7XHJcbiAgfVxyXG5cclxuICBidXR0b24ge1xyXG4gICAgbWFyZ2luLXRvcDogM3JlbTtcclxuICAgIHdpZHRoOiAxMDAlO1xyXG4gICAgbWF4LXdpZHRoOiAxOHJlbTtcclxuICB9XHJcbn0iXX0= */"

/***/ }),

/***/ "./src/app/add-contacts/add-contacts.component.ts":
/*!********************************************************!*\
  !*** ./src/app/add-contacts/add-contacts.component.ts ***!
  \********************************************************/
/*! exports provided: AddContactsComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "AddContactsComponent", function() { return AddContactsComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
/* harmony import */ var _angular_router__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! @angular/router */ "./node_modules/@angular/router/fesm5/router.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};







var AddContactsComponent = /** @class */ (function () {
    function AddContactsComponent(route, backend, variablesService, modalService, ngZone, location) {
        var _this = this;
        this.route = route;
        this.backend = backend;
        this.variablesService = variablesService;
        this.modalService = modalService;
        this.ngZone = ngZone;
        this.location = location;
        this.addContactForm = new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"]({
            address: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [
                _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required,
                function (g) {
                    if (g.value) {
                        _this.backend.validateAddress(g.value, function (valid_status) {
                            _this.ngZone.run(function () {
                                if (valid_status === false) {
                                    g.setErrors(Object.assign({ address_not_valid: true }, g.errors));
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
                        return g.hasError('address_not_valid')
                            ? { address_not_valid: true }
                            : null;
                    }
                    return null;
                },
                function (g) {
                    var isDublicated = _this.variablesService.contacts.findIndex(function (contact) { return contact.address === g.value; });
                    if (isDublicated !== -1 && !(_this.id === isDublicated)) {
                        return { dublicated: true };
                    }
                    return null;
                }
            ]),
            notes: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [
                function (g) {
                    if (g.value) {
                        if (g.value.length > _this.variablesService.maxCommentLength) {
                            return { maxLength: true };
                        }
                        else {
                            return null;
                        }
                    }
                    else {
                        return null;
                    }
                }
            ]),
            name: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"]('', [
                _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].required,
                _angular_forms__WEBPACK_IMPORTED_MODULE_1__["Validators"].pattern(/^[\w\s-_.]{4,25}$/),
                function (g) {
                    if (g.value) {
                        var isDublicated = _this.variablesService.contacts.findIndex(function (contact) { return contact.name === g.value.trim(); });
                        if (isDublicated !== -1 && !(_this.id === isDublicated)) {
                            return { dublicated: true };
                        }
                        return null;
                    }
                }
            ])
        });
    }
    AddContactsComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.queryRouting = this.route.queryParams.subscribe(function (params) {
            if (params.id) {
                _this.id = parseInt(params.id, 10);
                _this.addContactForm.reset({
                    name: _this.variablesService.contacts[params.id]['name'],
                    address: _this.variablesService.contacts[params.id]['address'],
                    notes: _this.variablesService.contacts[params.id]['notes']
                });
            }
            else {
                _this.addContactForm.reset({
                    name: _this.variablesService.newContact['name'],
                    address: _this.variablesService.newContact['address'],
                    notes: _this.variablesService.newContact['notes']
                });
            }
        });
    };
    AddContactsComponent.prototype.add = function () {
        var _this = this;
        if (!this.variablesService.appPass) {
            this.modalService.prepareModal('error', 'CONTACTS.FORM_ERRORS.SET_MASTER_PASSWORD');
        }
        else {
            if (this.addContactForm.valid) {
                this.backend.validateAddress(this.addContactForm.get('address').value, function (valid_status) {
                    if (valid_status === false) {
                        _this.ngZone.run(function () {
                            _this.addContactForm
                                .get('address')
                                .setErrors({ address_not_valid: true });
                        });
                    }
                    else {
                        if (_this.id || _this.id === 0) {
                            _this.variablesService.contacts.forEach(function (contact, index) {
                                if (index === _this.id) {
                                    contact.name = _this.addContactForm.get('name').value.trim();
                                    contact.address = _this.addContactForm.get('address').value;
                                    contact.notes =
                                        _this.addContactForm.get('notes').value || '';
                                }
                            });
                            _this.backend.storeSecureAppData();
                            _this.backend.getContactAlias();
                            _this.modalService.prepareModal('success', 'CONTACTS.SUCCESS_SAVE');
                        }
                        else {
                            _this.variablesService.contacts.push({
                                name: _this.addContactForm.get('name').value.trim(),
                                address: _this.addContactForm.get('address').value,
                                notes: _this.addContactForm.get('notes').value || ''
                            });
                            _this.backend.storeSecureAppData();
                            _this.backend.getContactAlias();
                            _this.modalService.prepareModal('success', 'CONTACTS.SUCCESS_SENT');
                            _this.variablesService.newContact = {
                                name: null,
                                address: null,
                                notes: null
                            };
                            _this.addContactForm.reset({
                                name: null,
                                address: null,
                                notes: null
                            });
                        }
                    }
                });
            }
        }
    };
    AddContactsComponent.prototype.back = function () {
        this.location.back();
    };
    AddContactsComponent.prototype.ngOnDestroy = function () {
        if (!this.id) {
            this.variablesService.newContact = {
                name: this.addContactForm.get('name').value,
                address: this.addContactForm.get('address').value,
                notes: this.addContactForm.get('notes').value
            };
        }
        this.queryRouting.unsubscribe();
    };
    AddContactsComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-add-contacts',
            template: __webpack_require__(/*! ./add-contacts.component.html */ "./src/app/add-contacts/add-contacts.component.html"),
            styles: [__webpack_require__(/*! ./add-contacts.component.scss */ "./src/app/add-contacts/add-contacts.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_router__WEBPACK_IMPORTED_MODULE_6__["ActivatedRoute"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__["ModalService"],
            _angular_core__WEBPACK_IMPORTED_MODULE_0__["NgZone"],
            _angular_common__WEBPACK_IMPORTED_MODULE_5__["Location"]])
    ], AddContactsComponent);
    return AddContactsComponent;
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
/* harmony import */ var _contacts_contacts_component__WEBPACK_IMPORTED_MODULE_22__ = __webpack_require__(/*! ./contacts/contacts.component */ "./src/app/contacts/contacts.component.ts");
/* harmony import */ var _add_contacts_add_contacts_component__WEBPACK_IMPORTED_MODULE_23__ = __webpack_require__(/*! ./add-contacts/add-contacts.component */ "./src/app/add-contacts/add-contacts.component.ts");
/* harmony import */ var _contact_send_contact_send_component__WEBPACK_IMPORTED_MODULE_24__ = __webpack_require__(/*! ./contact-send/contact-send.component */ "./src/app/contact-send/contact-send.component.ts");
/* harmony import */ var _export_import_export_import_component__WEBPACK_IMPORTED_MODULE_25__ = __webpack_require__(/*! ./export-import/export-import.component */ "./src/app/export-import/export-import.component.ts");
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
        path: 'contacts',
        component: _contacts_contacts_component__WEBPACK_IMPORTED_MODULE_22__["ContactsComponent"]
    },
    {
        path: 'add-contacts',
        component: _add_contacts_add_contacts_component__WEBPACK_IMPORTED_MODULE_23__["AddContactsComponent"]
    },
    {
        path: 'edit-contacts/:id',
        component: _add_contacts_add_contacts_component__WEBPACK_IMPORTED_MODULE_23__["AddContactsComponent"]
    },
    {
        path: 'contact-send/:id',
        component: _contact_send_contact_send_component__WEBPACK_IMPORTED_MODULE_24__["ContactSendComponent"]
    },
    {
        path: 'import',
        component: _export_import_export_import_component__WEBPACK_IMPORTED_MODULE_25__["ExportImportComponent"]
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

module.exports = "<app-sidebar *ngIf=\"variablesService.appLogin\"></app-sidebar>\r\n\r\n<div class=\"app-content scrolled-content\">\r\n  <router-outlet *ngIf=\"[0, 1, 2].indexOf(variablesService.daemon_state) !== -1\"></router-outlet>\r\n  <div class=\"preloader\" *ngIf=\"[3, 4, 5].indexOf(variablesService.daemon_state) !== -1\">\r\n    <span *ngIf=\"variablesService.daemon_state === 3\">{{ 'SIDEBAR.SYNCHRONIZATION.LOADING' | translate }}</span>\r\n    <span *ngIf=\"variablesService.daemon_state === 4\">{{ 'SIDEBAR.SYNCHRONIZATION.ERROR' | translate }}</span>\r\n    <span *ngIf=\"variablesService.daemon_state === 5\">{{ 'SIDEBAR.SYNCHRONIZATION.COMPLETE' | translate }}</span>\r\n    <span class=\"loading-bar\"></span>\r\n  </div>\r\n</div>\r\n<context-menu #allContextMenu>\r\n  <ng-template contextMenuItem (execute)=\"contextMenuCopy($event.item)\">{{ 'CONTEXT_MENU.COPY' | translate }}</ng-template>\r\n  <ng-template contextMenuItem (execute)=\"contextMenuPaste($event.item)\">{{ 'CONTEXT_MENU.PASTE' | translate }}</ng-template>\r\n  <ng-template contextMenuItem (execute)=\"contextMenuSelect($event.item)\">{{ 'CONTEXT_MENU.SELECT' | translate }}</ng-template>\r\n</context-menu>\r\n\r\n<context-menu #onlyCopyContextMenu>\r\n  <ng-template contextMenuItem (execute)=\"contextMenuOnlyCopy($event.item)\">{{ 'CONTEXT_MENU.COPY' | translate }}</ng-template>\r\n</context-menu>\r\n\r\n<context-menu #pasteSelectContextMenu>\r\n  <ng-template contextMenuItem (execute)=\"contextMenuPaste($event.item)\">{{ 'CONTEXT_MENU.PASTE' | translate }}</ng-template>\r\n  <ng-template contextMenuItem (execute)=\"contextMenuSelect($event.item)\">{{ 'CONTEXT_MENU.SELECT' | translate }}</ng-template>\r\n</context-menu>\r\n\r\n\r\n<app-open-wallet-modal *ngIf=\"needOpenWallets.length\" [wallets]=\"needOpenWallets\"></app-open-wallet-modal>\r\n"

/***/ }),

/***/ "./src/app/app.component.scss":
/*!************************************!*\
  !*** ./src/app/app.component.scss ***!
  \************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "/*\r\n* Implementation of themes\r\n*/\n.app-content {\n  display: flex;\n  overflow-x: overlay;\n  overflow-y: hidden;\n  width: 100%; }\n.app-content .preloader {\n    align-self: center;\n    color: #fff;\n    font-size: 2rem;\n    margin: 0 auto;\n    text-align: center;\n    width: 50%; }\n.app-content .preloader .loading-bar {\n      display: block;\n      -webkit-animation: move 5s linear infinite;\n              animation: move 5s linear infinite;\n      background-image: -webkit-gradient(linear, 0 0, 100% 100%, color-stop(0.125, rgba(0, 0, 0, 0.15)), color-stop(0.125, transparent), color-stop(0.25, transparent), color-stop(0.25, rgba(0, 0, 0, 0.1)), color-stop(0.375, rgba(0, 0, 0, 0.1)), color-stop(0.375, transparent), color-stop(0.5, transparent), color-stop(0.5, rgba(0, 0, 0, 0.15)), color-stop(0.625, rgba(0, 0, 0, 0.15)), color-stop(0.625, transparent), color-stop(0.75, transparent), color-stop(0.75, rgba(0, 0, 0, 0.1)), color-stop(0.875, rgba(0, 0, 0, 0.1)), color-stop(0.875, transparent), to(transparent)), -webkit-gradient(linear, 0 100%, 100% 0, color-stop(0.125, rgba(0, 0, 0, 0.3)), color-stop(0.125, transparent), color-stop(0.25, transparent), color-stop(0.25, rgba(0, 0, 0, 0.25)), color-stop(0.375, rgba(0, 0, 0, 0.25)), color-stop(0.375, transparent), color-stop(0.5, transparent), color-stop(0.5, rgba(0, 0, 0, 0.3)), color-stop(0.625, rgba(0, 0, 0, 0.3)), color-stop(0.625, transparent), color-stop(0.75, transparent), color-stop(0.75, rgba(0, 0, 0, 0.25)), color-stop(0.875, rgba(0, 0, 0, 0.25)), color-stop(0.875, transparent), to(transparent));\n      background-size: 10rem 10rem;\n      margin-top: 2rem;\n      width: 100%;\n      height: 1rem; }\n@-webkit-keyframes move {\n  0% {\n    background-position: 100% -10rem; }\n  100% {\n    background-position: 100% 10rem; } }\n@keyframes move {\n  0% {\n    background-position: 100% -10rem; }\n  100% {\n    background-position: 100% 10rem; } }\n\r\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2Fzc2V0c1xcc2Nzc1xcYmFzZVxcX21peGlucy5zY3NzIiwiYXBwL2FwcC5jb21wb25lbnQuc2NzcyIsImFwcC9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcYXBwLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQThFQTs7Q0M1RUM7QUNBRDtFQUNFLGFBQWE7RUFDYixtQkFBbUI7RUFDbkIsa0JBQWtCO0VBQ2xCLFdBQVcsRUFBQTtBQUpiO0lBT0ksa0JBQWtCO0lBQ2xCLFdBQVc7SUFDWCxlQUFlO0lBQ2YsY0FBYztJQUNkLGtCQUFrQjtJQUNsQixVQUFVLEVBQUE7QUFaZDtNQWVNLGNBQWM7TUFDZCwwQ0FBa0M7Y0FBbEMsa0NBQWtDO01BQ2xDLCtsQ0FzQkc7TUFDSCw0QkFBNEI7TUFDNUIsZ0JBQWdCO01BQ2hCLFdBQVc7TUFDWCxZQUFZLEVBQUE7QUFJaEI7RUFDRTtJQUNFLGdDQUFnQyxFQUFBO0VBRWxDO0lBQ0UsK0JBQStCLEVBQUEsRUFBQTtBQUxuQztFQUNFO0lBQ0UsZ0NBQWdDLEVBQUE7RUFFbEM7SUFDRSwrQkFBK0IsRUFBQSxFQUFBIiwiZmlsZSI6ImFwcC9hcHAuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyJAbWl4aW4gdGV4dC10cnVuY2F0ZSB7XHJcbiAgb3ZlcmZsb3c6IGhpZGRlbjtcclxuICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcclxuICB3aGl0ZS1zcGFjZTogbm93cmFwO1xyXG59XHJcbkBtaXhpbiB0ZXh0V3JhcCB7XHJcbiAgd2hpdGUtc3BhY2U6IG5vcm1hbDtcclxuICBvdmVyZmxvdy13cmFwOiBicmVhay13b3JkO1xyXG4gIHdvcmQtd3JhcDogYnJlYWstd29yZDtcclxuICB3b3JkLWJyZWFrOiBicmVhay1hbGw7XHJcbiAgbGluZS1icmVhazogc3RyaWN0O1xyXG4gIC13ZWJraXQtaHlwaGVuczogYXV0bztcclxuICAtbXMtaHlwaGVuczogYXV0bztcclxuICBoeXBoZW5zOiBhdXRvO1xyXG59XHJcbkBtaXhpbiBjb3ZlckJveCB7XHJcblx0cG9zaXRpb246IGFic29sdXRlO1xyXG5cdHRvcDogMDtcclxuXHRsZWZ0OiAwO1xyXG5cdHdpZHRoOiAxMDAlO1xyXG5cdGhlaWdodDogMTAwJTtcclxufVxyXG5AbWl4aW4gYWJzICgkdG9wOiBhdXRvLCAkcmlnaHQ6IGF1dG8sICRib3R0b206IGF1dG8sICRsZWZ0OiBhdXRvKSB7XHJcbiAgdG9wOiAkdG9wO1xyXG4gIHJpZ2h0OiAkcmlnaHQ7XHJcbiAgYm90dG9tOiAkYm90dG9tO1xyXG4gIGxlZnQ6ICRsZWZ0O1xyXG4gIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxufVxyXG5AbWl4aW4gY292ZXJJbWcge1xyXG5cdGJhY2tncm91bmQtcmVwZWF0OiBuby1yZXBlYXQ7XHJcblx0LXdlYmtpdC1iYWNrZ3JvdW5kLXNpemU6IGNvdmVyO1xyXG5cdC1vLWJhY2tncm91bmQtc2l6ZTogY292ZXI7XHJcblx0YmFja2dyb3VuZC1zaXplOiBjb3ZlcjtcclxuXHRiYWNrZ3JvdW5kLXBvc2l0aW9uOiA1MCUgNTAlO1xyXG59XHJcbkBtaXhpbiB2YWxpbmdCb3gge1xyXG5cdHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuXHR0b3A6ICA1MCU7XHJcblx0bGVmdDogNTAlO1xyXG5cdHRyYW5zZm9ybTogdHJhbnNsYXRlKC01MCUsIC01MCUpO1xyXG59XHJcbkBtaXhpbiB1blNlbGVjdCB7XHJcblx0LXdlYmtpdC10b3VjaC1jb2xsb3V0OiBub25lO1xyXG4gIC13ZWJraXQtdXNlci1zZWxlY3Q6IG5vbmU7XHJcbiAgLWtodG1sLXVzZXItc2VsZWN0OiBub25lO1xyXG4gIC1tb3otdXNlci1zZWxlY3Q6IG5vbmU7XHJcbiAgLW1zLXVzZXItc2VsZWN0OiBub25lO1xyXG4gIHVzZXItc2VsZWN0OiBub25lO1xyXG59XHJcbkBtaXhpbiBtYXgxMTk5IHsgLy8gbWFrZXQgMTE3MVxyXG4gIEBtZWRpYSAobWF4LXdpZHRoOiAxMTk5cHgpIHsgQGNvbnRlbnQ7IH1cclxufVxyXG5AbWl4aW4gbWF4MTE3MCB7IC8vIG1ha2V0cyA5OTJcclxuICBAbWVkaWEgKG1heC13aWR0aDogMTE3MHB4KSB7IEBjb250ZW50OyB9XHJcbn1cclxuQG1peGluIG1heDk5MSB7IC8vIG1ha2V0cyA3NjJcclxuICBAbWVkaWEgKG1heC13aWR0aDogOTkxcHgpIHsgQGNvbnRlbnQ7IH1cclxufVxyXG5AbWl4aW4gbWF4NzYxIHsgLy8gbWFrZXRzIDU3NlxyXG4gIEBtZWRpYSAobWF4LXdpZHRoOiA3NjFweCkgeyBAY29udGVudDsgfVxyXG59XHJcbkBtaXhpbiBtYXg1NzUgeyAvLyBtYWtldHMgNDAwXHJcbiAgQG1lZGlhIChtYXgtd2lkdGg6IDU3NXB4KSB7IEBjb250ZW50OyB9XHJcbn1cclxuQG1peGluIG1vYmlsZSB7XHJcbiAgQG1lZGlhIChtYXgtd2lkdGg6IDM5OXB4KSB7IEBjb250ZW50OyB9XHJcbn1cclxuQG1peGluIGljb0NlbnRlciB7XHJcbiAgICBiYWNrZ3JvdW5kLXJlcGVhdDogbm8tcmVwZWF0O1xyXG4gICAgYmFja2dyb3VuZC1wb3NpdGlvbjogY2VudGVyIGNlbnRlcjtcclxufVxyXG5AbWl4aW4gcHNldWRvICgkZGlzcGxheTogYmxvY2ssICRwb3M6IGFic29sdXRlLCAkY29udGVudDogJycpe1xyXG4gIGNvbnRlbnQ6ICRjb250ZW50O1xyXG4gIGRpc3BsYXk6ICRkaXNwbGF5O1xyXG4gIHBvc2l0aW9uOiAkcG9zO1xyXG59XHJcblxyXG4vKlxyXG4qIEltcGxlbWVudGF0aW9uIG9mIHRoZW1lc1xyXG4qL1xyXG5AbWl4aW4gdGhlbWlmeSgkdGhlbWVzOiAkdGhlbWVzKSB7XHJcbiAgQGVhY2ggJHRoZW1lLCAkbWFwIGluICR0aGVtZXMge1xyXG4gICAgLnRoZW1lLSN7JHRoZW1lfSAmIHtcclxuICAgICAgJHRoZW1lLW1hcDogKCkgIWdsb2JhbDtcclxuICAgICAgQGVhY2ggJGtleSwgJHN1Ym1hcCBpbiAkbWFwIHtcclxuICAgICAgICAkdmFsdWU6IG1hcC1nZXQobWFwLWdldCgkdGhlbWVzLCAkdGhlbWUpLCAnI3ska2V5fScpO1xyXG4gICAgICAgICR0aGVtZS1tYXA6IG1hcC1tZXJnZSgkdGhlbWUtbWFwLCAoJGtleTogJHZhbHVlKSkgIWdsb2JhbDtcclxuICAgICAgfVxyXG4gICAgICBAY29udGVudDtcclxuICAgICAgJHRoZW1lLW1hcDogbnVsbCAhZ2xvYmFsO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG5cclxuQGZ1bmN0aW9uIHRoZW1lZCgka2V5KSB7XHJcbiAgQHJldHVybiBtYXAtZ2V0KCR0aGVtZS1tYXAsICRrZXkpO1xyXG59XHJcbiIsIi8qXHJcbiogSW1wbGVtZW50YXRpb24gb2YgdGhlbWVzXHJcbiovXG4uYXBwLWNvbnRlbnQge1xuICBkaXNwbGF5OiBmbGV4O1xuICBvdmVyZmxvdy14OiBvdmVybGF5O1xuICBvdmVyZmxvdy15OiBoaWRkZW47XG4gIHdpZHRoOiAxMDAlOyB9XG4gIC5hcHAtY29udGVudCAucHJlbG9hZGVyIHtcbiAgICBhbGlnbi1zZWxmOiBjZW50ZXI7XG4gICAgY29sb3I6ICNmZmY7XG4gICAgZm9udC1zaXplOiAycmVtO1xuICAgIG1hcmdpbjogMCBhdXRvO1xuICAgIHRleHQtYWxpZ246IGNlbnRlcjtcbiAgICB3aWR0aDogNTAlOyB9XG4gICAgLmFwcC1jb250ZW50IC5wcmVsb2FkZXIgLmxvYWRpbmctYmFyIHtcbiAgICAgIGRpc3BsYXk6IGJsb2NrO1xuICAgICAgYW5pbWF0aW9uOiBtb3ZlIDVzIGxpbmVhciBpbmZpbml0ZTtcbiAgICAgIGJhY2tncm91bmQtaW1hZ2U6IC13ZWJraXQtZ3JhZGllbnQobGluZWFyLCAwIDAsIDEwMCUgMTAwJSwgY29sb3Itc3RvcCgwLjEyNSwgcmdiYSgwLCAwLCAwLCAwLjE1KSksIGNvbG9yLXN0b3AoMC4xMjUsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCgwLjI1LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC4yNSwgcmdiYSgwLCAwLCAwLCAwLjEpKSwgY29sb3Itc3RvcCgwLjM3NSwgcmdiYSgwLCAwLCAwLCAwLjEpKSwgY29sb3Itc3RvcCgwLjM3NSwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKDAuNSwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKDAuNSwgcmdiYSgwLCAwLCAwLCAwLjE1KSksIGNvbG9yLXN0b3AoMC42MjUsIHJnYmEoMCwgMCwgMCwgMC4xNSkpLCBjb2xvci1zdG9wKDAuNjI1LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC43NSwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKDAuNzUsIHJnYmEoMCwgMCwgMCwgMC4xKSksIGNvbG9yLXN0b3AoMC44NzUsIHJnYmEoMCwgMCwgMCwgMC4xKSksIGNvbG9yLXN0b3AoMC44NzUsIHRyYW5zcGFyZW50KSwgdG8odHJhbnNwYXJlbnQpKSwgLXdlYmtpdC1ncmFkaWVudChsaW5lYXIsIDAgMTAwJSwgMTAwJSAwLCBjb2xvci1zdG9wKDAuMTI1LCByZ2JhKDAsIDAsIDAsIDAuMykpLCBjb2xvci1zdG9wKDAuMTI1LCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoMC4yNSwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKDAuMjUsIHJnYmEoMCwgMCwgMCwgMC4yNSkpLCBjb2xvci1zdG9wKDAuMzc1LCByZ2JhKDAsIDAsIDAsIDAuMjUpKSwgY29sb3Itc3RvcCgwLjM3NSwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKDAuNSwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKDAuNSwgcmdiYSgwLCAwLCAwLCAwLjMpKSwgY29sb3Itc3RvcCgwLjYyNSwgcmdiYSgwLCAwLCAwLCAwLjMpKSwgY29sb3Itc3RvcCgwLjYyNSwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKDAuNzUsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCgwLjc1LCByZ2JhKDAsIDAsIDAsIDAuMjUpKSwgY29sb3Itc3RvcCgwLjg3NSwgcmdiYSgwLCAwLCAwLCAwLjI1KSksIGNvbG9yLXN0b3AoMC44NzUsIHRyYW5zcGFyZW50KSwgdG8odHJhbnNwYXJlbnQpKTtcbiAgICAgIGJhY2tncm91bmQtc2l6ZTogMTByZW0gMTByZW07XG4gICAgICBtYXJnaW4tdG9wOiAycmVtO1xuICAgICAgd2lkdGg6IDEwMCU7XG4gICAgICBoZWlnaHQ6IDFyZW07IH1cblxuQGtleWZyYW1lcyBtb3ZlIHtcbiAgMCUge1xuICAgIGJhY2tncm91bmQtcG9zaXRpb246IDEwMCUgLTEwcmVtOyB9XG4gIDEwMCUge1xuICAgIGJhY2tncm91bmQtcG9zaXRpb246IDEwMCUgMTByZW07IH0gfVxuIiwiQGltcG9ydCAnfnNyYy9hc3NldHMvc2Nzcy9iYXNlL21peGlucyc7XHJcblxyXG4uYXBwLWNvbnRlbnQge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgb3ZlcmZsb3cteDogb3ZlcmxheTtcclxuICBvdmVyZmxvdy15OiBoaWRkZW47XHJcbiAgd2lkdGg6IDEwMCU7XHJcblxyXG4gIC5wcmVsb2FkZXIge1xyXG4gICAgYWxpZ24tc2VsZjogY2VudGVyO1xyXG4gICAgY29sb3I6ICNmZmY7XHJcbiAgICBmb250LXNpemU6IDJyZW07XHJcbiAgICBtYXJnaW46IDAgYXV0bztcclxuICAgIHRleHQtYWxpZ246IGNlbnRlcjtcclxuICAgIHdpZHRoOiA1MCU7XHJcblxyXG4gICAgLmxvYWRpbmctYmFyIHtcclxuICAgICAgZGlzcGxheTogYmxvY2s7XHJcbiAgICAgIGFuaW1hdGlvbjogbW92ZSA1cyBsaW5lYXIgaW5maW5pdGU7XHJcbiAgICAgIGJhY2tncm91bmQtaW1hZ2U6XHJcbiAgICAgICAgLXdlYmtpdC1ncmFkaWVudChcclxuICAgICAgICAgICAgbGluZWFyLCAwIDAsIDEwMCUgMTAwJSxcclxuICAgICAgICAgICAgY29sb3Itc3RvcCguMTI1LCByZ2JhKDAsIDAsIDAsIC4xNSkpLCBjb2xvci1zdG9wKC4xMjUsIHRyYW5zcGFyZW50KSxcclxuICAgICAgICAgICAgY29sb3Itc3RvcCguMjUwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjI1MCwgcmdiYSgwLCAwLCAwLCAuMTApKSxcclxuICAgICAgICAgICAgY29sb3Itc3RvcCguMzc1LCByZ2JhKDAsIDAsIDAsIC4xMCkpLCBjb2xvci1zdG9wKC4zNzUsIHRyYW5zcGFyZW50KSxcclxuICAgICAgICAgICAgY29sb3Itc3RvcCguNTAwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjUwMCwgcmdiYSgwLCAwLCAwLCAuMTUpKSxcclxuICAgICAgICAgICAgY29sb3Itc3RvcCguNjI1LCByZ2JhKDAsIDAsIDAsIC4xNSkpLCBjb2xvci1zdG9wKC42MjUsIHRyYW5zcGFyZW50KSxcclxuICAgICAgICAgICAgY29sb3Itc3RvcCguNzUwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjc1MCwgcmdiYSgwLCAwLCAwLCAuMTApKSxcclxuICAgICAgICAgICAgY29sb3Itc3RvcCguODc1LCByZ2JhKDAsIDAsIDAsIC4xMCkpLCBjb2xvci1zdG9wKC44NzUsIHRyYW5zcGFyZW50KSxcclxuICAgICAgICAgICAgdG8odHJhbnNwYXJlbnQpXHJcbiAgICAgICAgKSxcclxuICAgICAgICAtd2Via2l0LWdyYWRpZW50KFxyXG4gICAgICAgICAgICBsaW5lYXIsIDAgMTAwJSwgMTAwJSAwLFxyXG4gICAgICAgICAgICBjb2xvci1zdG9wKC4xMjUsIHJnYmEoMCwgMCwgMCwgLjMwKSksIGNvbG9yLXN0b3AoLjEyNSwgdHJhbnNwYXJlbnQpLFxyXG4gICAgICAgICAgICBjb2xvci1zdG9wKC4yNTAsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCguMjUwLCByZ2JhKDAsIDAsIDAsIC4yNSkpLFxyXG4gICAgICAgICAgICBjb2xvci1zdG9wKC4zNzUsIHJnYmEoMCwgMCwgMCwgLjI1KSksIGNvbG9yLXN0b3AoLjM3NSwgdHJhbnNwYXJlbnQpLFxyXG4gICAgICAgICAgICBjb2xvci1zdG9wKC41MDAsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCguNTAwLCByZ2JhKDAsIDAsIDAsIC4zMCkpLFxyXG4gICAgICAgICAgICBjb2xvci1zdG9wKC42MjUsIHJnYmEoMCwgMCwgMCwgLjMwKSksIGNvbG9yLXN0b3AoLjYyNSwgdHJhbnNwYXJlbnQpLFxyXG4gICAgICAgICAgICBjb2xvci1zdG9wKC43NTAsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCguNzUwLCByZ2JhKDAsIDAsIDAsIC4yNSkpLFxyXG4gICAgICAgICAgICBjb2xvci1zdG9wKC44NzUsIHJnYmEoMCwgMCwgMCwgLjI1KSksIGNvbG9yLXN0b3AoLjg3NSwgdHJhbnNwYXJlbnQpLFxyXG4gICAgICAgICAgICB0byh0cmFuc3BhcmVudClcclxuICAgICAgICApO1xyXG4gICAgICBiYWNrZ3JvdW5kLXNpemU6IDEwcmVtIDEwcmVtO1xyXG4gICAgICBtYXJnaW4tdG9wOiAycmVtO1xyXG4gICAgICB3aWR0aDogMTAwJTtcclxuICAgICAgaGVpZ2h0OiAxcmVtO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgQGtleWZyYW1lcyBtb3ZlIHtcclxuICAgIDAlIHtcclxuICAgICAgYmFja2dyb3VuZC1wb3NpdGlvbjogMTAwJSAtMTByZW07XHJcbiAgICB9XHJcbiAgICAxMDAlIHtcclxuICAgICAgYmFja2dyb3VuZC1wb3NpdGlvbjogMTAwJSAxMHJlbTtcclxuICAgIH1cclxuICB9XHJcbn1cclxuIl19 */"

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
                    _this.backend.getContactAlias();
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
/* harmony import */ var _send_modal_send_modal_component__WEBPACK_IMPORTED_MODULE_49__ = __webpack_require__(/*! ./send-modal/send-modal.component */ "./src/app/send-modal/send-modal.component.ts");
/* harmony import */ var _contacts_contacts_component__WEBPACK_IMPORTED_MODULE_50__ = __webpack_require__(/*! ./contacts/contacts.component */ "./src/app/contacts/contacts.component.ts");
/* harmony import */ var _add_contacts_add_contacts_component__WEBPACK_IMPORTED_MODULE_51__ = __webpack_require__(/*! ./add-contacts/add-contacts.component */ "./src/app/add-contacts/add-contacts.component.ts");
/* harmony import */ var _contact_send_contact_send_component__WEBPACK_IMPORTED_MODULE_52__ = __webpack_require__(/*! ./contact-send/contact-send.component */ "./src/app/contact-send/contact-send.component.ts");
/* harmony import */ var _export_import_export_import_component__WEBPACK_IMPORTED_MODULE_53__ = __webpack_require__(/*! ./export-import/export-import.component */ "./src/app/export-import/export-import.component.ts");
/* harmony import */ var ngx_papaparse__WEBPACK_IMPORTED_MODULE_54__ = __webpack_require__(/*! ngx-papaparse */ "./node_modules/ngx-papaparse/fesm5/ngx-papaparse.js");
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
                _helpers_directives_input_disable_selection_input_disable_selection_directive__WEBPACK_IMPORTED_MODULE_48__["InputDisableSelectionDirective"],
                _send_modal_send_modal_component__WEBPACK_IMPORTED_MODULE_49__["SendModalComponent"],
                _contacts_contacts_component__WEBPACK_IMPORTED_MODULE_50__["ContactsComponent"],
                _add_contacts_add_contacts_component__WEBPACK_IMPORTED_MODULE_51__["AddContactsComponent"],
                _contact_send_contact_send_component__WEBPACK_IMPORTED_MODULE_52__["ContactSendComponent"],
                _export_import_export_import_component__WEBPACK_IMPORTED_MODULE_53__["ExportImportComponent"]
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
                ngx_papaparse__WEBPACK_IMPORTED_MODULE_54__["PapaParseModule"],
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
                _helpers_directives_modal_container_modal_container_component__WEBPACK_IMPORTED_MODULE_41__["ModalContainerComponent"],
                _send_modal_send_modal_component__WEBPACK_IMPORTED_MODULE_49__["SendModalComponent"]
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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span [routerLink]=\"['/wallet/' + wallet.wallet_id + '/history']\">{{ wallet.name }}</span>\r\n      <span>{{ 'BREADCRUMBS.ASSIGN_ALIAS' | translate }}</span>\r\n    </div>\r\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <form class=\"form-assign\" [formGroup]=\"assignForm\">\r\n\r\n    <div class=\"input-block alias-name\">\r\n      <label for=\"alias-name\" tooltip=\"{{ 'ASSIGN_ALIAS.NAME.TOOLTIP' | translate }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip assign-alias-tooltip\" [delay]=\"50\">\r\n        {{ 'ASSIGN_ALIAS.NAME.LABEL' | translate }}\r\n      </label>\r\n      <input type=\"text\" id=\"alias-name\" formControlName=\"name\" placeholder=\"{{ 'ASSIGN_ALIAS.NAME.PLACEHOLDER' | translate }}\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"assignForm.controls['name'].invalid && (assignForm.controls['name'].dirty || assignForm.controls['name'].touched)\">\r\n        <div *ngIf=\"assignForm.controls['name'].errors['required']\">\r\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NAME_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"assignForm.controls['name'].errors['pattern'] && assignForm.get('name').value.length > 6 && assignForm.get('name').value.length <= 25\">\r\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NAME_WRONG' | translate }}\r\n        </div>\r\n        <div *ngIf=\"assignForm.get('name').value.length <= 6 || assignForm.get('name').value.length > 25\">\r\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NAME_LENGTH' | translate }}\r\n        </div>\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"alias.exists\">\r\n        <div>\r\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NAME_EXISTS' | translate }}\r\n        </div>\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"notEnoughMoney\">\r\n        <div>\r\n          {{ 'ASSIGN_ALIAS.FORM_ERRORS.NO_MONEY' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block textarea\">\r\n      <label for=\"alias-comment\" tooltip=\"{{ 'ASSIGN_ALIAS.COMMENT.TOOLTIP' | translate }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip assign-alias-tooltip\" [delay]=\"50\">\r\n        {{ 'ASSIGN_ALIAS.COMMENT.LABEL' | translate }}\r\n      </label>\r\n      <textarea id=\"alias-comment\"\r\n                class=\"scrolled-content\"\r\n                formControlName=\"comment\"\r\n                placeholder=\"{{ 'ASSIGN_ALIAS.COMMENT.PLACEHOLDER' | translate }}\"\r\n                [maxLength]=\"variablesService.maxCommentLength\"\r\n                (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      </textarea>\r\n      <div class=\"error-block\" *ngIf=\"assignForm.get('comment').value.length >= variablesService.maxCommentLength\">\r\n        {{ 'ASSIGN_ALIAS.FORM_ERRORS.MAX_LENGTH' | translate }}\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"alias-cost\">{{ \"ASSIGN_ALIAS.COST\" | translate : {value: alias.price | intToMoney, currency: variablesService.defaultCurrency} }}</div>\r\n\r\n    <div class=\"wrap-buttons\">\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"assignAlias()\" [disabled]=\"!assignForm.valid || !canRegister || notEnoughMoney\">{{ 'ASSIGN_ALIAS.BUTTON_ASSIGN' | translate }}</button>\r\n    </div>\r\n\r\n  </form>\r\n\r\n</div>\r\n\r\n"

/***/ }),

/***/ "./src/app/assign-alias/assign-alias.component.scss":
/*!**********************************************************!*\
  !*** ./src/app/assign-alias/assign-alias.component.scss ***!
  \**********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-assign {\n  margin: 2.4rem 0; }\n  .form-assign .alias-name {\n    width: 50%; }\n  .form-assign .alias-cost {\n    font-size: 1.3rem;\n    margin-top: 2rem; }\n  .form-assign .wrap-buttons {\n    display: flex;\n    justify-content: space-between;\n    margin: 2.5rem -0.7rem; }\n  .form-assign .wrap-buttons button {\n      margin: 0 0.7rem;\n      width: 15rem; }\n  .assign-alias-tooltip {\n  font-size: 1.3rem;\n  line-height: 2rem;\n  padding: 1rem 1.5rem;\n  max-width: 46rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9hc3NpZ24tYWxpYXMvRDpcXFByb2plY3RzX25vd1xcWkFOT1xcemFub1xcc3JjXFxndWlcXHF0LWRhZW1vblxcaHRtbF9zb3VyY2VcXHNyYy9hcHBcXGFzc2lnbi1hbGlhc1xcYXNzaWduLWFsaWFzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZ0JBQWdCLEVBQUE7RUFEbEI7SUFJSSxVQUFVLEVBQUE7RUFKZDtJQVFJLGlCQUFpQjtJQUNqQixnQkFBZ0IsRUFBQTtFQVRwQjtJQWFJLGFBQWE7SUFDYiw4QkFBOEI7SUFDOUIsc0JBQXNCLEVBQUE7RUFmMUI7TUFrQk0sZ0JBQWdCO01BQ2hCLFlBQVksRUFBQTtFQUtsQjtFQUNFLGlCQUFpQjtFQUNqQixpQkFBaUI7RUFDakIsb0JBQW9CO0VBQ3BCLGdCQUFnQixFQUFBIiwiZmlsZSI6ImFwcC9hc3NpZ24tYWxpYXMvYXNzaWduLWFsaWFzLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tYXNzaWduIHtcclxuICBtYXJnaW46IDIuNHJlbSAwO1xyXG5cclxuICAuYWxpYXMtbmFtZSB7XHJcbiAgICB3aWR0aDogNTAlO1xyXG4gIH1cclxuXHJcbiAgLmFsaWFzLWNvc3Qge1xyXG4gICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICBtYXJnaW4tdG9wOiAycmVtO1xyXG4gIH1cclxuXHJcbiAgLndyYXAtYnV0dG9ucyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAganVzdGlmeS1jb250ZW50OiBzcGFjZS1iZXR3ZWVuO1xyXG4gICAgbWFyZ2luOiAyLjVyZW0gLTAuN3JlbTtcclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBtYXJnaW46IDAgMC43cmVtO1xyXG4gICAgICB3aWR0aDogMTVyZW07XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcblxyXG4uYXNzaWduLWFsaWFzLXRvb2x0aXAge1xyXG4gIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gIGxpbmUtaGVpZ2h0OiAycmVtO1xyXG4gIHBhZGRpbmc6IDFyZW0gMS41cmVtO1xyXG4gIG1heC13aWR0aDogNDZyZW07XHJcbn1cclxuIl19 */"

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

/***/ "./src/app/contact-send/contact-send.component.html":
/*!**********************************************************!*\
  !*** ./src/app/contact-send/contact-send.component.html ***!
  \**********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content scrolled-content\">\n  <div class=\"head\">\n    <div class=\"breadcrumbs\">\n      <span [routerLink]=\"['/contacts']\">{{\n        'CONTACTS.TITLE' | translate\n      }}</span>\n      <span>{{ 'CONTACTS.SEND' | translate }}</span>\n    </div>\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n      <i class=\"icon back\"></i>\n      <span>{{ 'COMMON.BACK' | translate }}</span>\n    </button>\n  </div>\n\n  <div>\n    <div class=\"wallets-selection\">\n      <div class=\"input-block\">\n        <label>\n          {{ 'CONTACTS.SEND_FROM' | translate }}\n        </label>\n        <ng-select\n          class=\"custom-select\"\n          [items]=\"this.variablesService.wallets\"\n          [(ngModel)]=\"this.variablesService.selectWallet\"\n          bindValue=\"wallet_id\"\n          bindLabel=\"name\"\n          [clearable]=\"false\"\n          [searchable]=\"false\"\n        >\n        </ng-select>\n      </div>\n      <button [routerLink]=\"['/main']\">\n        {{ 'CONTACTS.OPEN_ADD_WALLET' | translate }}\n      </button>\n    </div>\n\n    <div class=\"input-block\">\n      <label for=\"address\">{{ 'CONTACTS.SEND_TO' | translate }}</label>\n      <input type=\"text\" id=\"address\" [ngModel]=\"address\" [readonly]=\"true\"/>\n    </div>\n  </div>\n  <button class=\"blue-button\" [routerLink]=\"['/wallet/' + this.variablesService.selectWallet + '/send']\" [queryParams]=\"{send: true}\" (click)=\"goToWallet(this.variablesService.selectWallet)\"\n    [disabled]=\"!(this.variablesService.selectWallet === 0 || this.variablesService.selectWallet)\">{{ 'CONTACTS.BUTTON.GO_TO_WALLET' | translate }}</button>\n</div>\n"

/***/ }),

/***/ "./src/app/contact-send/contact-send.component.scss":
/*!**********************************************************!*\
  !*** ./src/app/contact-send/contact-send.component.scss ***!
  \**********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".wallets-selection {\n  display: flex;\n  align-items: center;\n  margin-top: 2rem; }\n  .wallets-selection .input-block {\n    width: 18rem; }\n  .wallets-selection button {\n    padding: 2rem;\n    background: transparent;\n    border: none;\n    outline: none; }\n  .input-block {\n  width: 44rem; }\n  .input-block input {\n    overflow: hidden;\n    text-overflow: ellipsis; }\n  .blue-button {\n  margin-top: 2.5rem;\n  width: 100%;\n  max-width: 18rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9jb250YWN0LXNlbmQvRDpcXFByb2plY3RzX25vd1xcWkFOT1xcemFub1xcc3JjXFxndWlcXHF0LWRhZW1vblxcaHRtbF9zb3VyY2VcXHNyYy9hcHBcXGNvbnRhY3Qtc2VuZFxcY29udGFjdC1zZW5kLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsYUFBYTtFQUNiLG1CQUFtQjtFQUNuQixnQkFBZ0IsRUFBQTtFQUhsQjtJQU1JLFlBQVksRUFBQTtFQU5oQjtJQVVJLGFBQWE7SUFDYix1QkFBdUI7SUFDdkIsWUFBWTtJQUNaLGFBQWEsRUFBQTtFQUlqQjtFQUNFLFlBQVksRUFBQTtFQURkO0lBSUksZ0JBQWdCO0lBQ2hCLHVCQUF1QixFQUFBO0VBSTNCO0VBQ0Usa0JBQWtCO0VBQ2xCLFdBQVc7RUFDWCxnQkFBZ0IsRUFBQSIsImZpbGUiOiJhcHAvY29udGFjdC1zZW5kL2NvbnRhY3Qtc2VuZC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIi53YWxsZXRzLXNlbGVjdGlvbiB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gIG1hcmdpbi10b3A6IDJyZW07XHJcbiAgXHJcbiAgLmlucHV0LWJsb2NrIHtcclxuICAgIHdpZHRoOiAxOHJlbTtcclxuICB9XHJcblxyXG4gIGJ1dHRvbiB7XHJcbiAgICBwYWRkaW5nOiAycmVtO1xyXG4gICAgYmFja2dyb3VuZDogdHJhbnNwYXJlbnQ7XHJcbiAgICBib3JkZXI6IG5vbmU7XHJcbiAgICBvdXRsaW5lOiBub25lO1xyXG4gIH1cclxufVxyXG5cclxuLmlucHV0LWJsb2NrIHtcclxuICB3aWR0aDogNDRyZW07XHJcblxyXG4gIGlucHV0IHtcclxuICAgIG92ZXJmbG93OiBoaWRkZW47XHJcbiAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcclxuICB9XHJcbn1cclxuXHJcbi5ibHVlLWJ1dHRvbiB7XHJcbiAgbWFyZ2luLXRvcDogMi41cmVtO1xyXG4gIHdpZHRoOiAxMDAlO1xyXG4gIG1heC13aWR0aDogMThyZW07XHJcbn1cclxuXHJcblxyXG4iXX0= */"

/***/ }),

/***/ "./src/app/contact-send/contact-send.component.ts":
/*!********************************************************!*\
  !*** ./src/app/contact-send/contact-send.component.ts ***!
  \********************************************************/
/*! exports provided: ContactSendComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ContactSendComponent", function() { return ContactSendComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
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




var ContactSendComponent = /** @class */ (function () {
    function ContactSendComponent(location, variablesService, route) {
        this.location = location;
        this.variablesService = variablesService;
        this.route = route;
    }
    ContactSendComponent.prototype.ngOnInit = function () {
        var _this = this;
        this.queryRouting = this.route.queryParams.subscribe(function (params) {
            if (params.address) {
                _this.address = params.address;
            }
        });
    };
    ContactSendComponent.prototype.goToWallet = function (id) {
        this.variablesService.setCurrentWallet(id);
        this.variablesService.currentWallet.send_data['address'] = this.address;
    };
    ContactSendComponent.prototype.back = function () {
        this.location.back();
    };
    ContactSendComponent.prototype.ngOnDestroy = function () {
        this.queryRouting.unsubscribe();
    };
    ContactSendComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-contact-send',
            template: __webpack_require__(/*! ./contact-send.component.html */ "./src/app/contact-send/contact-send.component.html"),
            styles: [__webpack_require__(/*! ./contact-send.component.scss */ "./src/app/contact-send/contact-send.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_common__WEBPACK_IMPORTED_MODULE_1__["Location"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"],
            _angular_router__WEBPACK_IMPORTED_MODULE_3__["ActivatedRoute"]])
    ], ContactSendComponent);
    return ContactSendComponent;
}());



/***/ }),

/***/ "./src/app/contacts/contacts.component.html":
/*!**************************************************!*\
  !*** ./src/app/contacts/contacts.component.html ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content scrolled-content\">\n  <div>\n    <div class=\"head\">\n      <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n        <i class=\"icon back\"></i>\n        <span>{{ 'COMMON.BACK' | translate }}</span>\n      </button>\n    </div>\n\n    <h3 class=\"contacts-title\">{{ 'CONTACTS.TITLE' | translate }}</h3>\n\n    <div class=\"wrap-table\">\n      <ng-container>\n        <table\n          *ngIf=\"this.variablesService.contacts.length !== 0; else emptyList\"\n        >\n          <thead>\n            <tr #head (window:resize)=\"calculateWidth()\">\n              <th>{{ 'CONTACTS.TABLE.NAME' | translate }}</th>\n              <th>{{ 'CONTACTS.TABLE.ALIAS' | translate }}</th>\n              <th>{{ 'CONTACTS.TABLE.ADDRESS' | translate }}</th>\n              <th>{{ 'CONTACTS.TABLE.NOTES' | translate }}</th>\n              <th></th>\n            </tr>\n          </thead>\n          <tbody>\n            <ng-container\n              *ngFor=\"\n                let contact of this.variablesService.contacts;\n                let i = index\n              \"\n            >\n              <tr>\n                <td>\n                  {{ contact.name }}\n                </td>\n                <td>\n                  <ng-container *ngIf=\"contact.alias\">\n                    <span\n                      class=\"alias\"\n                      (click)=\"openInBrowser(contact.alias)\"\n                      >{{ contact.alias }}</span\n                    >\n                  </ng-container>\n                </td>\n                <td class=\"remote-address\">\n                  {{ contact.address }}\n                </td>\n                <td class=\"remote-notes\">\n                  {{ contact.notes }}\n                </td>\n                <td>\n                  <div class=\"button-wrapper\">\n                    <button\n                      [routerLink]=\"['/contact-send/' + i]\"\n                      [queryParams]=\"{ address: contact.address }\"\n                    >\n                      <i class=\"icon transfer\"></i>\n                      <span>{{ 'CONTACTS.BUTTON.SEND' | translate }}</span>\n                    </button>\n                    <button\n                      [routerLink]=\"['/edit-contacts/' + i]\"\n                      [queryParams]=\"{ id: i }\"\n                    >\n                      <i class=\"icon edit\"></i>\n                      <span>{{ 'CONTACTS.BUTTON.EDIT' | translate }}</span>\n                    </button>\n                    <button (click)=\"delete(i)\">\n                      <i class=\"icon delete\"></i>\n                      <span>{{ 'CONTACTS.BUTTON.DELETE' | translate }}</span>\n                    </button>\n                  </div>\n                </td>\n              </tr>\n            </ng-container>\n          </tbody>\n        </table>\n      </ng-container>\n\n      <ng-template #emptyList>\n        <div class=\"empty-list\">\n          {{ 'CONTACTS.TABLE.EMPTY' | translate }}\n        </div>\n      </ng-template>\n    </div>\n\n    <button [routerLink]=\"['/add-contacts']\" class=\"blue-button\">\n      {{ 'CONTACTS.BUTTON.ADD' | translate }}\n    </button>\n\n    <div class=\"footer\">\n      <button type=\"button\" class=\"import-btn\" [routerLink]=\"['/import']\">\n        <i class=\"icon import\"></i>\n        <span>{{ 'CONTACTS.BUTTON.IMPORT_EXPORT' | translate }}</span>\n      </button>\n    </div>\n  </div>\n</div>\n"

/***/ }),

/***/ "./src/app/contacts/contacts.component.scss":
/*!**************************************************!*\
  !*** ./src/app/contacts/contacts.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  min-width: 95rem;\n  width: 100%;\n  height: 100%; }\n\n.head {\n  justify-content: flex-end; }\n\n.contacts-title {\n  font-size: 1.7rem; }\n\n.wrap-table {\n  margin: 1rem -3rem; }\n\n.wrap-table table tbody tr td {\n    padding: 0 3rem 0 1rem;\n    overflow: hidden;\n    text-overflow: ellipsis; }\n\n.wrap-table table tbody tr td:first-child {\n      max-width: 10rem;\n      padding: 0 3rem 0 3rem; }\n\n.wrap-table table tbody tr td:nth-child(2) {\n      max-width: 10rem; }\n\n.wrap-table table tbody tr td .alias {\n      cursor: pointer; }\n\n.wrap-table table tbody tr td .button-wrapper {\n      display: flex; }\n\n.wrap-table table tbody tr td .button-wrapper button {\n        display: flex;\n        align-items: center;\n        background: transparent;\n        border: none;\n        font-size: 1.3rem;\n        font-weight: 400;\n        line-height: 3rem;\n        outline: none;\n        padding: 0;\n        height: auto;\n        margin-right: 1.8rem; }\n\n.wrap-table table tbody tr td .button-wrapper button .icon {\n          cursor: pointer;\n          margin-right: 0.8rem;\n          width: 1.7rem;\n          height: 1.7rem; }\n\n.wrap-table table tbody tr td .button-wrapper button .icon.edit {\n            -webkit-mask: url('edit.svg') no-repeat center;\n                    mask: url('edit.svg') no-repeat center; }\n\n.wrap-table table tbody tr td .button-wrapper button .icon.transfer {\n            -webkit-mask: url('send.svg') no-repeat center;\n                    mask: url('send.svg') no-repeat center; }\n\n.wrap-table table tbody tr td .button-wrapper button .icon.delete {\n            -webkit-mask: url('delete.svg') no-repeat center;\n                    mask: url('delete.svg') no-repeat center; }\n\n.wrap-table .empty-list {\n    margin: 2.5rem 3rem; }\n\n.blue-button {\n  width: 100%;\n  max-width: 18rem;\n  margin-top: 3rem; }\n\n.footer {\n  position: absolute;\n  bottom: 3rem;\n  font-size: 1.3rem; }\n\n.footer .import-btn {\n    display: flex;\n    align-items: center;\n    background-color: transparent;\n    font-size: inherit;\n    font-weight: 400;\n    line-height: 1.3rem;\n    padding: 0;\n    height: auto; }\n\n.footer .import-btn .icon {\n      margin-right: 0.7rem;\n      -webkit-mask: url('import-export.svg') no-repeat center;\n              mask: url('import-export.svg') no-repeat center;\n      width: 0.9rem;\n      height: 0.9rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9jb250YWN0cy9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcY29udGFjdHNcXGNvbnRhY3RzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZ0JBQWdCO0VBQ2hCLFdBQVc7RUFDWCxZQUFZLEVBQUE7O0FBR2Q7RUFDRSx5QkFBeUIsRUFBQTs7QUFHM0I7RUFDRSxpQkFBaUIsRUFBQTs7QUFHbkI7RUFDRSxrQkFBa0IsRUFBQTs7QUFEcEI7SUFVVSxzQkFBc0I7SUFDdEIsZ0JBQWdCO0lBQ2hCLHVCQUF1QixFQUFBOztBQVpqQztNQWVZLGdCQUFnQjtNQUNoQixzQkFBc0IsRUFBQTs7QUFoQmxDO01Bb0JZLGdCQUFnQixFQUFBOztBQXBCNUI7TUF3QlksZUFBZSxFQUFBOztBQXhCM0I7TUE0QlksYUFBYSxFQUFBOztBQTVCekI7UUErQmMsYUFBYTtRQUNiLG1CQUFtQjtRQUNuQix1QkFBdUI7UUFDdkIsWUFBWTtRQUNaLGlCQUFpQjtRQUNqQixnQkFBZ0I7UUFDaEIsaUJBQWlCO1FBQ2pCLGFBQWE7UUFDYixVQUFVO1FBQ1YsWUFBWTtRQUNaLG9CQUFvQixFQUFBOztBQXpDbEM7VUE0Q2dCLGVBQWU7VUFDZixvQkFBb0I7VUFDcEIsYUFBYTtVQUNiLGNBQWMsRUFBQTs7QUEvQzlCO1lBa0RrQiw4Q0FBdUQ7b0JBQXZELHNDQUF1RCxFQUFBOztBQWxEekU7WUFzRGtCLDhDQUF1RDtvQkFBdkQsc0NBQXVELEVBQUE7O0FBdER6RTtZQTBEa0IsZ0RBQXlEO29CQUF6RCx3Q0FBeUQsRUFBQTs7QUExRDNFO0lBcUVJLG1CQUFtQixFQUFBOztBQUl2QjtFQUNFLFdBQVc7RUFDWCxnQkFBZ0I7RUFDaEIsZ0JBQWdCLEVBQUE7O0FBSWxCO0VBQ0Usa0JBQWtCO0VBQ2xCLFlBQVk7RUFDWixpQkFBaUIsRUFBQTs7QUFIbkI7SUFNSSxhQUFhO0lBQ2IsbUJBQW1CO0lBQ25CLDZCQUE2QjtJQUM3QixrQkFBa0I7SUFDbEIsZ0JBQWdCO0lBQ2hCLG1CQUFtQjtJQUNuQixVQUFVO0lBQ1YsWUFBWSxFQUFBOztBQWJoQjtNQWdCTSxvQkFBb0I7TUFDcEIsdURBQWdFO2NBQWhFLCtDQUFnRTtNQUNoRSxhQUFhO01BQ2IsY0FBYyxFQUFBIiwiZmlsZSI6ImFwcC9jb250YWN0cy9jb250YWN0cy5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBtaW4td2lkdGg6IDk1cmVtO1xyXG4gIHdpZHRoOiAxMDAlO1xyXG4gIGhlaWdodDogMTAwJTtcclxufVxyXG5cclxuLmhlYWQge1xyXG4gIGp1c3RpZnktY29udGVudDogZmxleC1lbmQ7XHJcbn1cclxuXHJcbi5jb250YWN0cy10aXRsZSB7XHJcbiAgZm9udC1zaXplOiAxLjdyZW07XHJcbn1cclxuXHJcbi53cmFwLXRhYmxlIHtcclxuICBtYXJnaW46IDFyZW0gLTNyZW07XHJcblxyXG4gIHRhYmxlIHtcclxuXHJcbiAgICB0Ym9keXtcclxuXHJcbiAgICAgIHRyIHtcclxuXHJcbiAgICAgICAgdGQge1xyXG4gICAgICAgICAgcGFkZGluZzogMCAzcmVtIDAgMXJlbTtcclxuICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XHJcbiAgICAgICAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcclxuXHJcbiAgICAgICAgICAmOmZpcnN0LWNoaWxkIHtcclxuICAgICAgICAgICAgbWF4LXdpZHRoOiAxMHJlbTtcclxuICAgICAgICAgICAgcGFkZGluZzogMCAzcmVtIDAgM3JlbTtcclxuICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAmOm50aC1jaGlsZCgyKSB7XHJcbiAgICAgICAgICAgIG1heC13aWR0aDogMTByZW07XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgLmFsaWFzIHtcclxuICAgICAgICAgICAgY3Vyc29yOiBwb2ludGVyO1xyXG4gICAgICAgICAgfVxyXG5cclxuICAgICAgICAgIC5idXR0b24td3JhcHBlciB7XHJcbiAgICAgICAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgICAgICAgIFxyXG4gICAgICAgICAgICBidXR0b24ge1xyXG4gICAgICAgICAgICAgIGRpc3BsYXk6IGZsZXg7ICBcclxuICAgICAgICAgICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICAgICAgICAgIGJhY2tncm91bmQ6IHRyYW5zcGFyZW50O1xyXG4gICAgICAgICAgICAgIGJvcmRlcjogbm9uZTtcclxuICAgICAgICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcclxuICAgICAgICAgICAgICBmb250LXdlaWdodDogNDAwO1xyXG4gICAgICAgICAgICAgIGxpbmUtaGVpZ2h0OiAzcmVtO1xyXG4gICAgICAgICAgICAgIG91dGxpbmU6IG5vbmU7XHJcbiAgICAgICAgICAgICAgcGFkZGluZzogMDtcclxuICAgICAgICAgICAgICBoZWlnaHQ6IGF1dG87XHJcbiAgICAgICAgICAgICAgbWFyZ2luLXJpZ2h0OiAxLjhyZW07XHJcbiAgICAgICAgICBcclxuICAgICAgICAgICAgICAuaWNvbiB7XHJcbiAgICAgICAgICAgICAgICBjdXJzb3I6IHBvaW50ZXI7XHJcbiAgICAgICAgICAgICAgICBtYXJnaW4tcmlnaHQ6IDAuOHJlbTtcclxuICAgICAgICAgICAgICAgIHdpZHRoOiAxLjdyZW07XHJcbiAgICAgICAgICAgICAgICBoZWlnaHQ6IDEuN3JlbTtcclxuICAgICAgICAgICAgXHJcbiAgICAgICAgICAgICAgICAmLmVkaXQge1xyXG4gICAgICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2VkaXQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgICAgICAgICAgfVxyXG4gICAgICAgICAgICBcclxuICAgICAgICAgICAgICAgICYudHJhbnNmZXIge1xyXG4gICAgICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3NlbmQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgICAgICAgICAgfVxyXG4gICAgICAgIFxyXG4gICAgICAgICAgICAgICAgJi5kZWxldGUge1xyXG4gICAgICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2RlbGV0ZS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgICAgICAgICAgICB9XHJcbiAgICAgICAgICAgICAgfVxyXG4gICAgICAgICAgICB9XHJcbiAgICAgICAgICB9XHJcbiAgICAgICAgfVxyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG4gIFxyXG4gIC5lbXB0eS1saXN0IHtcclxuICAgIG1hcmdpbjogMi41cmVtIDNyZW07XHJcbiAgfSAgXHJcbn1cclxuXHJcbi5ibHVlLWJ1dHRvbiB7XHJcbiAgd2lkdGg6IDEwMCU7XHJcbiAgbWF4LXdpZHRoOiAxOHJlbTtcclxuICBtYXJnaW4tdG9wOiAzcmVtO1xyXG59XHJcblxyXG5cclxuLmZvb3RlciB7XHJcbiAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gIGJvdHRvbTogM3JlbTtcclxuICBmb250LXNpemU6IDEuM3JlbTtcclxuICBcclxuICAuaW1wb3J0LWJ0biB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGJhY2tncm91bmQtY29sb3I6IHRyYW5zcGFyZW50O1xyXG4gICAgZm9udC1zaXplOiBpbmhlcml0O1xyXG4gICAgZm9udC13ZWlnaHQ6IDQwMDtcclxuICAgIGxpbmUtaGVpZ2h0OiAxLjNyZW07XHJcbiAgICBwYWRkaW5nOiAwO1xyXG4gICAgaGVpZ2h0OiBhdXRvO1xyXG5cclxuICAgIC5pY29uIHtcclxuICAgICAgbWFyZ2luLXJpZ2h0OiAwLjdyZW07XHJcbiAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvaW1wb3J0LWV4cG9ydC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIHdpZHRoOiAwLjlyZW07XHJcbiAgICAgIGhlaWdodDogMC45cmVtO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

/***/ }),

/***/ "./src/app/contacts/contacts.component.ts":
/*!************************************************!*\
  !*** ./src/app/contacts/contacts.component.ts ***!
  \************************************************/
/*! exports provided: ContactsComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ContactsComponent", function() { return ContactsComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
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




var ContactsComponent = /** @class */ (function () {
    function ContactsComponent(location, variablesService, backend) {
        this.location = location;
        this.variablesService = variablesService;
        this.backend = backend;
        this.calculatedWidth = [];
    }
    ContactsComponent.prototype.ngOnInit = function () {
        this.backend.getContactAlias();
    };
    ContactsComponent.prototype.delete = function (index) {
        if (this.variablesService.appPass) {
            this.variablesService.contacts.splice(index, 1);
            this.backend.storeSecureAppData();
        }
    };
    ContactsComponent.prototype.calculateWidth = function () {
        this.calculatedWidth = [];
        this.calculatedWidth.push(this.head.nativeElement.childNodes[0].clientWidth);
        this.calculatedWidth.push(this.head.nativeElement.childNodes[1].clientWidth +
            this.head.nativeElement.childNodes[2].clientWidth);
        this.calculatedWidth.push(this.head.nativeElement.childNodes[3].clientWidth);
        this.calculatedWidth.push(this.head.nativeElement.childNodes[4].clientWidth);
    };
    ContactsComponent.prototype.openInBrowser = function (alias) {
        if (alias !== null) {
            this.backend.openUrlInBrowser("explorer.zano.org/aliases/" + alias.slice(1));
        }
    };
    ContactsComponent.prototype.back = function () {
        this.location.back();
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["ViewChild"])('head'),
        __metadata("design:type", _angular_core__WEBPACK_IMPORTED_MODULE_0__["ElementRef"])
    ], ContactsComponent.prototype, "head", void 0);
    ContactsComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-contacts',
            template: __webpack_require__(/*! ./contacts.component.html */ "./src/app/contacts/contacts.component.html"),
            styles: [__webpack_require__(/*! ./contacts.component.scss */ "./src/app/contacts/contacts.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_common__WEBPACK_IMPORTED_MODULE_1__["Location"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_3__["BackendService"]])
    ], ContactsComponent);
    return ContactsComponent;
}());



/***/ }),

/***/ "./src/app/contracts/contracts.component.html":
/*!****************************************************!*\
  !*** ./src/app/contracts/contracts.component.html ***!
  \****************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"empty-contracts\" *ngIf=\"!variablesService.currentWallet.contracts.length\">\r\n  <span>{{ 'CONTRACTS.EMPTY' | translate }}</span>\r\n</div>\r\n\r\n<div class=\"wrap-table scrolled-content\" *ngIf=\"variablesService.currentWallet.contracts.length\">\r\n\r\n  <table class=\"contracts-table\">\r\n    <thead>\r\n    <tr>\r\n      <th>{{ 'CONTRACTS.CONTRACTS' | translate }}</th>\r\n      <th>{{ 'CONTRACTS.DATE' | translate }}</th>\r\n      <th>{{ 'CONTRACTS.AMOUNT' | translate }}</th>\r\n      <th>{{ 'CONTRACTS.STATUS' | translate }}</th>\r\n      <th>{{ 'CONTRACTS.COMMENTS' | translate }}</th>\r\n    </tr>\r\n    </thead>\r\n    <tbody>\r\n    <tr *ngFor=\"let item of sortedArrayContracts\" [routerLink]=\"'/wallet/' + walletId + '/purchase/' + item.contract_id\">\r\n      <td>\r\n        <div class=\"contract\">\r\n          <i class=\"icon alert\" *ngIf=\"!item.is_new\"></i>\r\n          <i class=\"icon new\" *ngIf=\"item.is_new\"></i>\r\n          <i class=\"icon\" [class.purchase]=\"item.is_a\" [class.sell]=\"!item.is_a\"></i>\r\n          <span tooltip=\"{{ item.private_detailes.t }}\" placement=\"top-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{item.private_detailes.t}}</span>\r\n        </div>\r\n      </td>\r\n      <td>\r\n        <div>{{item.timestamp * 1000 | date : 'dd-MM-yyyy HH:mm'}}</div>\r\n      </td>\r\n      <td>\r\n        <div>{{item.private_detailes.to_pay | intToMoney}} {{variablesService.defaultCurrency}}</div>\r\n      </td>\r\n      <td>\r\n        <div class=\"status\" [class.error-text]=\"item.state === 4\" tooltip=\"{{item.state | contractStatusMessages : item.is_a}}\" placement=\"top\" tooltipClass=\"table-tooltip\" [delay]=\"500\">\r\n          {{item.state | contractStatusMessages : item.is_a}}\r\n        </div>\r\n      </td>\r\n      <td>\r\n        <div class=\"comment\" tooltip=\"{{ item.private_detailes.c }}\" placement=\"top-right\" tooltipClass=\"table-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">\r\n          {{item.private_detailes.c}}\r\n        </div>\r\n      </td>\r\n    </tr>\r\n    </tbody>\r\n  </table>\r\n\r\n</div>\r\n\r\n<div class=\"contracts-buttons\">\r\n  <button type=\"button\" class=\"blue-button\" [routerLink]=\"'/wallet/' + walletId + '/purchase'\">{{ 'CONTRACTS.PURCHASE_BUTTON' | translate }}</button>\r\n  <button type=\"button\" class=\"blue-button\" disabled>{{ 'CONTRACTS.LISTING_BUTTON' | translate }}</button>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/contracts/contracts.component.scss":
/*!****************************************************!*\
  !*** ./src/app/contracts/contracts.component.scss ***!
  \****************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.empty-contracts {\n  font-size: 1.5rem; }\n\n.wrap-table {\n  margin: -3rem -3rem 0 -3rem;\n  overflow-x: auto; }\n\n.wrap-table table tbody tr {\n    cursor: pointer;\n    outline: none !important; }\n\n.wrap-table table tbody tr .contract {\n      position: relative;\n      display: flex;\n      align-items: center; }\n\n.wrap-table table tbody tr .contract .icon {\n        flex-shrink: 0; }\n\n.wrap-table table tbody tr .contract .icon.new, .wrap-table table tbody tr .contract .icon.alert {\n          position: absolute;\n          top: 0; }\n\n.wrap-table table tbody tr .contract .icon.new {\n          left: -2.3rem;\n          -webkit-mask: url('new.svg') no-repeat center;\n                  mask: url('new.svg') no-repeat center;\n          width: 1.7rem;\n          height: 1.7rem; }\n\n.wrap-table table tbody tr .contract .icon.alert {\n          top: 0.2rem;\n          left: -2.1rem;\n          -webkit-mask: url('alert.svg') no-repeat center;\n                  mask: url('alert.svg') no-repeat center;\n          width: 1.2rem;\n          height: 1.2rem; }\n\n.wrap-table table tbody tr .contract .icon.purchase, .wrap-table table tbody tr .contract .icon.sell {\n          margin-right: 1rem;\n          width: 1.5rem;\n          height: 1.5rem; }\n\n.wrap-table table tbody tr .contract .icon.purchase {\n          -webkit-mask: url('purchase.svg') no-repeat center;\n                  mask: url('purchase.svg') no-repeat center; }\n\n.wrap-table table tbody tr .contract .icon.sell {\n          -webkit-mask: url('sell.svg') no-repeat center;\n                  mask: url('sell.svg') no-repeat center; }\n\n.wrap-table table tbody tr .contract span {\n        text-overflow: ellipsis;\n        overflow: hidden; }\n\n.wrap-table table tbody tr .status, .wrap-table table tbody tr .comment {\n      display: inline-block;\n      text-overflow: ellipsis;\n      overflow: hidden;\n      max-width: 100%; }\n\n.contracts-buttons {\n  display: flex;\n  margin: 3rem 0;\n  width: 50%; }\n\n.contracts-buttons button {\n    flex: 0 1 50%;\n    margin-right: 1.5rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9jb250cmFjdHMvRDpcXFByb2plY3RzX25vd1xcWkFOT1xcemFub1xcc3JjXFxndWlcXHF0LWRhZW1vblxcaHRtbF9zb3VyY2VcXHNyYy9hcHBcXGNvbnRyYWN0c1xcY29udHJhY3RzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsV0FBVyxFQUFBOztBQUdiO0VBQ0UsaUJBQWlCLEVBQUE7O0FBR25CO0VBQ0UsMkJBQTJCO0VBQzNCLGdCQUFnQixFQUFBOztBQUZsQjtJQVNRLGVBQWU7SUFDZix3QkFBd0IsRUFBQTs7QUFWaEM7TUFhVSxrQkFBa0I7TUFDbEIsYUFBYTtNQUNiLG1CQUFtQixFQUFBOztBQWY3QjtRQWtCWSxjQUFjLEVBQUE7O0FBbEIxQjtVQXFCYyxrQkFBa0I7VUFDbEIsTUFBTSxFQUFBOztBQXRCcEI7VUEwQmMsYUFBYTtVQUNiLDZDQUFzRDtrQkFBdEQscUNBQXNEO1VBQ3RELGFBQWE7VUFDYixjQUFjLEVBQUE7O0FBN0I1QjtVQWlDYyxXQUFXO1VBQ1gsYUFBYTtVQUNiLCtDQUF3RDtrQkFBeEQsdUNBQXdEO1VBQ3hELGFBQWE7VUFDYixjQUFjLEVBQUE7O0FBckM1QjtVQXlDYyxrQkFBa0I7VUFDbEIsYUFBYTtVQUNiLGNBQWMsRUFBQTs7QUEzQzVCO1VBK0NjLGtEQUEyRDtrQkFBM0QsMENBQTJELEVBQUE7O0FBL0N6RTtVQW1EYyw4Q0FBdUQ7a0JBQXZELHNDQUF1RCxFQUFBOztBQW5EckU7UUF3RFksdUJBQXVCO1FBQ3ZCLGdCQUFnQixFQUFBOztBQXpENUI7TUE4RFUscUJBQXFCO01BQ3JCLHVCQUF1QjtNQUN2QixnQkFBZ0I7TUFDaEIsZUFBZSxFQUFBOztBQU96QjtFQUNFLGFBQWE7RUFDYixjQUFjO0VBQ2QsVUFBVSxFQUFBOztBQUhaO0lBTUksYUFBYTtJQUNiLG9CQUFvQixFQUFBIiwiZmlsZSI6ImFwcC9jb250cmFjdHMvY29udHJhY3RzLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xyXG4gIHdpZHRoOiAxMDAlO1xyXG59XHJcblxyXG4uZW1wdHktY29udHJhY3RzIHtcclxuICBmb250LXNpemU6IDEuNXJlbTtcclxufVxyXG5cclxuLndyYXAtdGFibGUge1xyXG4gIG1hcmdpbjogLTNyZW0gLTNyZW0gMCAtM3JlbTtcclxuICBvdmVyZmxvdy14OiBhdXRvO1xyXG5cclxuICB0YWJsZSB7XHJcblxyXG4gICAgdGJvZHkge1xyXG5cclxuICAgICAgdHIge1xyXG4gICAgICAgIGN1cnNvcjogcG9pbnRlcjtcclxuICAgICAgICBvdXRsaW5lOiBub25lICFpbXBvcnRhbnQ7XHJcblxyXG4gICAgICAgIC5jb250cmFjdCB7XHJcbiAgICAgICAgICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgICAgICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuXHJcbiAgICAgICAgICAuaWNvbiB7XHJcbiAgICAgICAgICAgIGZsZXgtc2hyaW5rOiAwO1xyXG5cclxuICAgICAgICAgICAgJi5uZXcsICYuYWxlcnQge1xyXG4gICAgICAgICAgICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuICAgICAgICAgICAgICB0b3A6IDA7XHJcbiAgICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAgICYubmV3IHtcclxuICAgICAgICAgICAgICBsZWZ0OiAtMi4zcmVtO1xyXG4gICAgICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvbmV3LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICAgICAgICB3aWR0aDogMS43cmVtO1xyXG4gICAgICAgICAgICAgIGhlaWdodDogMS43cmVtO1xyXG4gICAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgICAmLmFsZXJ0IHtcclxuICAgICAgICAgICAgICB0b3A6IDAuMnJlbTtcclxuICAgICAgICAgICAgICBsZWZ0OiAtMi4xcmVtO1xyXG4gICAgICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvYWxlcnQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgICAgICAgIHdpZHRoOiAxLjJyZW07XHJcbiAgICAgICAgICAgICAgaGVpZ2h0OiAxLjJyZW07XHJcbiAgICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAgICYucHVyY2hhc2UsICYuc2VsbCB7XHJcbiAgICAgICAgICAgICAgbWFyZ2luLXJpZ2h0OiAxcmVtO1xyXG4gICAgICAgICAgICAgIHdpZHRoOiAxLjVyZW07XHJcbiAgICAgICAgICAgICAgaGVpZ2h0OiAxLjVyZW07XHJcbiAgICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAgICYucHVyY2hhc2Uge1xyXG4gICAgICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvcHVyY2hhc2Uuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgICAmLnNlbGwge1xyXG4gICAgICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvc2VsbC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgICAgICAgIH1cclxuICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICBzcGFuIHtcclxuICAgICAgICAgICAgdGV4dC1vdmVyZmxvdzogZWxsaXBzaXM7XHJcbiAgICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XHJcbiAgICAgICAgICB9XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAuc3RhdHVzLCAuY29tbWVudCB7XHJcbiAgICAgICAgICBkaXNwbGF5OiBpbmxpbmUtYmxvY2s7XHJcbiAgICAgICAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcclxuICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XHJcbiAgICAgICAgICBtYXgtd2lkdGg6IDEwMCU7XHJcbiAgICAgICAgfVxyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcblxyXG4uY29udHJhY3RzLWJ1dHRvbnMge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgbWFyZ2luOiAzcmVtIDA7XHJcbiAgd2lkdGg6IDUwJTtcclxuXHJcbiAgYnV0dG9uIHtcclxuICAgIGZsZXg6IDAgMSA1MCU7XHJcbiAgICBtYXJnaW4tcmlnaHQ6IDEuNXJlbTtcclxuICB9XHJcbn1cclxuIl19 */"

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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span [routerLink]=\"['/main']\">{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\r\n      <span>{{ 'BREADCRUMBS.CREATE_WALLET' | translate }}</span>\r\n    </div>\r\n    <button type=\"button\" class=\"back-btn\" [routerLink]=\"['/main']\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <form class=\"form-create\" [formGroup]=\"createForm\">\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-name\">{{ 'CREATE_WALLET.NAME' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [attr.readonly]=\"walletSaved ? '' : null\" [maxlength]=\"variablesService.maxWalletNameLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"createForm.controls['name'].invalid && (createForm.controls['name'].dirty || createForm.controls['name'].touched)\">\r\n        <div *ngIf=\"createForm.controls['name'].errors['required']\">\r\n          {{ 'CREATE_WALLET.FORM_ERRORS.NAME_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"createForm.controls['name'].errors['duplicate']\">\r\n          {{ 'CREATE_WALLET.FORM_ERRORS.NAME_DUPLICATE' | translate }}\r\n        </div>\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"createForm.get('name').value.length >= variablesService.maxWalletNameLength\">\r\n        {{ 'CREATE_WALLET.FORM_ERRORS.MAX_LENGTH' | translate }}\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-password\">{{ 'CREATE_WALLET.PASS' | translate }}</label>\r\n      <input type=\"password\" id=\"wallet-password\" formControlName=\"password\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"confirm-wallet-password\">{{ 'CREATE_WALLET.CONFIRM' | translate }}</label>\r\n      <input type=\"password\" id=\"confirm-wallet-password\" formControlName=\"confirm\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\r\n      <div class=\"error-block\" *ngIf=\"createForm.controls['password'].dirty && createForm.controls['confirm'].dirty && createForm.errors\">\r\n        <div *ngIf=\"createForm.errors['confirm_mismatch']\">\r\n          {{ 'CREATE_WALLET.FORM_ERRORS.CONFIRM_NOT_MATCH' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"wrap-buttons\">\r\n      <button type=\"button\" class=\"transparent-button\" *ngIf=\"walletSaved\" disabled><i class=\"icon\"></i>{{walletSavedName}}</button>\r\n      <button type=\"button\" class=\"blue-button select-button\" (click)=\"saveWallet()\" [disabled]=\"!createForm.valid\" *ngIf=\"!walletSaved\">{{ 'CREATE_WALLET.BUTTON_SELECT' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button create-button\" (click)=\"createWallet()\" [disabled]=\"!walletSaved\">{{ 'CREATE_WALLET.BUTTON_CREATE' | translate }}</button>\r\n    </div>\r\n\r\n  </form>\r\n\r\n</div>\r\n\r\n<app-progress-container [width]=\"progressWidth\" [labels]=\"['PROGRESS.ADD_WALLET', 'PROGRESS.SELECT_LOCATION', 'PROGRESS.CREATE_WALLET']\"></app-progress-container>\r\n"

/***/ }),

/***/ "./src/app/create-wallet/create-wallet.component.scss":
/*!************************************************************!*\
  !*** ./src/app/create-wallet/create-wallet.component.scss ***!
  \************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: relative; }\n\n.form-create {\n  margin: 2.4rem 0;\n  width: 50%; }\n\n.form-create .wrap-buttons {\n    display: flex;\n    margin: 2.5rem -0.7rem; }\n\n.form-create .wrap-buttons button {\n      margin: 0 0.7rem; }\n\n.form-create .wrap-buttons button.transparent-button {\n        flex-basis: 50%; }\n\n.form-create .wrap-buttons button.select-button {\n        flex-basis: 60%; }\n\n.form-create .wrap-buttons button.create-button {\n        flex: 1 1 50%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9jcmVhdGUtd2FsbGV0L0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxjcmVhdGUtd2FsbGV0XFxjcmVhdGUtd2FsbGV0LmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCLEVBQUE7O0FBR3BCO0VBQ0UsZ0JBQWdCO0VBQ2hCLFVBQVUsRUFBQTs7QUFGWjtJQUtJLGFBQWE7SUFDYixzQkFBc0IsRUFBQTs7QUFOMUI7TUFTTSxnQkFBZ0IsRUFBQTs7QUFUdEI7UUFZUSxlQUFlLEVBQUE7O0FBWnZCO1FBZ0JRLGVBQWUsRUFBQTs7QUFoQnZCO1FBb0JRLGFBQWEsRUFBQSIsImZpbGUiOiJhcHAvY3JlYXRlLXdhbGxldC9jcmVhdGUtd2FsbGV0LmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xyXG4gIHBvc2l0aW9uOiByZWxhdGl2ZTtcclxufVxyXG5cclxuLmZvcm0tY3JlYXRlIHtcclxuICBtYXJnaW46IDIuNHJlbSAwO1xyXG4gIHdpZHRoOiA1MCU7XHJcblxyXG4gIC53cmFwLWJ1dHRvbnMge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIG1hcmdpbjogMi41cmVtIC0wLjdyZW07XHJcblxyXG4gICAgYnV0dG9uIHtcclxuICAgICAgbWFyZ2luOiAwIDAuN3JlbTtcclxuXHJcbiAgICAgICYudHJhbnNwYXJlbnQtYnV0dG9uIHtcclxuICAgICAgICBmbGV4LWJhc2lzOiA1MCU7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYuc2VsZWN0LWJ1dHRvbiB7XHJcbiAgICAgICAgZmxleC1iYXNpczogNjAlO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLmNyZWF0ZS1idXR0b24ge1xyXG4gICAgICAgIGZsZXg6IDEgMSA1MCU7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcbn1cclxuIl19 */"

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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span [routerLink]=\"['/wallet/' + wallet.wallet_id + '/history']\">{{ wallet.name }}</span>\r\n      <span>{{ 'BREADCRUMBS.EDIT_ALIAS' | translate }}</span>\r\n    </div>\r\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <form class=\"form-edit\">\r\n\r\n    <div class=\"input-block alias-name\">\r\n      <label for=\"alias-name\">\r\n        {{ 'EDIT_ALIAS.NAME.LABEL' | translate }}\r\n      </label>\r\n      <input type=\"text\" id=\"alias-name\" [value]=\"alias.name\" placeholder=\"{{ 'EDIT_ALIAS.NAME.PLACEHOLDER' | translate }}\" readonly>\r\n    </div>\r\n\r\n    <div class=\"input-block textarea\">\r\n      <label for=\"alias-comment\">\r\n        {{ 'EDIT_ALIAS.COMMENT.LABEL' | translate }}\r\n      </label>\r\n      <textarea id=\"alias-comment\"\r\n                class=\"scrolled-content\"\r\n                [(ngModel)]=\"alias.comment\"\r\n                [ngModelOptions]=\"{standalone: true}\"\r\n                [maxlength]=\"variablesService.maxCommentLength\"\r\n                (contextmenu)=\"variablesService.onContextMenu($event)\"\r\n                placeholder=\"{{ 'EDIT_ALIAS.COMMENT.PLACEHOLDER' | translate }}\">\r\n      </textarea>\r\n      <div class=\"error-block\" *ngIf=\"alias.comment.length > 0 && notEnoughMoney\">\r\n        {{ 'EDIT_ALIAS.FORM_ERRORS.NO_MONEY' | translate }}\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"alias.comment.length >= variablesService.maxCommentLength\">\r\n        {{ 'EDIT_ALIAS.FORM_ERRORS.MAX_LENGTH' | translate }}\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"alias-cost\">{{ \"EDIT_ALIAS.COST\" | translate : {value: variablesService.default_fee, currency: variablesService.defaultCurrency} }}</div>\r\n\r\n    <div class=\"wrap-buttons\">\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"updateAlias()\" [disabled]=\"notEnoughMoney || (oldAliasComment === alias.comment) || alias.comment.length > variablesService.maxCommentLength\">{{ 'EDIT_ALIAS.BUTTON_EDIT' | translate }}</button>\r\n    </div>\r\n\r\n  </form>\r\n\r\n</div>\r\n\r\n\r\n"

/***/ }),

/***/ "./src/app/edit-alias/edit-alias.component.scss":
/*!******************************************************!*\
  !*** ./src/app/edit-alias/edit-alias.component.scss ***!
  \******************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-edit {\n  margin: 2.4rem 0; }\n  .form-edit .alias-name {\n    width: 50%; }\n  .form-edit .alias-cost {\n    font-size: 1.3rem;\n    margin-top: 2rem; }\n  .form-edit .wrap-buttons {\n    display: flex;\n    justify-content: space-between;\n    margin: 2.5rem -0.7rem; }\n  .form-edit .wrap-buttons button {\n      margin: 0 0.7rem;\n      width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9lZGl0LWFsaWFzL0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxlZGl0LWFsaWFzXFxlZGl0LWFsaWFzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZ0JBQWdCLEVBQUE7RUFEbEI7SUFJSSxVQUFVLEVBQUE7RUFKZDtJQVFJLGlCQUFpQjtJQUNqQixnQkFBZ0IsRUFBQTtFQVRwQjtJQWFJLGFBQWE7SUFDYiw4QkFBOEI7SUFDOUIsc0JBQXNCLEVBQUE7RUFmMUI7TUFrQk0sZ0JBQWdCO01BQ2hCLFlBQVksRUFBQSIsImZpbGUiOiJhcHAvZWRpdC1hbGlhcy9lZGl0LWFsaWFzLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tZWRpdCB7XHJcbiAgbWFyZ2luOiAyLjRyZW0gMDtcclxuXHJcbiAgLmFsaWFzLW5hbWUge1xyXG4gICAgd2lkdGg6IDUwJTtcclxuICB9XHJcblxyXG4gIC5hbGlhcy1jb3N0IHtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbWFyZ2luLXRvcDogMnJlbTtcclxuICB9XHJcblxyXG4gIC53cmFwLWJ1dHRvbnMge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICAgIG1hcmdpbjogMi41cmVtIC0wLjdyZW07XHJcblxyXG4gICAgYnV0dG9uIHtcclxuICAgICAgbWFyZ2luOiAwIDAuN3JlbTtcclxuICAgICAgd2lkdGg6IDE1cmVtO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

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

/***/ "./src/app/export-import/export-import.component.html":
/*!************************************************************!*\
  !*** ./src/app/export-import/export-import.component.html ***!
  \************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"content scrolled-content\">\n  <div>\n    <div class=\"head\">\n      <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\n        <i class=\"icon back\"></i>\n        <span>{{ 'COMMON.BACK' | translate }}</span>\n      </button>\n    </div>\n\n    <h3 class=\"contacts-title\">{{ 'CONTACTS.IMPORT_EXPORT' | translate }}</h3>\n\n    <div class=\"btn-wrapper\">\n      <button class=\"blue-button\" type=\"button\" (click)=\"import()\">\n        {{ 'CONTACTS.IMPORT' | translate }}\n      </button>\n      <button class=\"blue-button\" type=\"button\" (click)=\"export()\">\n        {{ 'CONTACTS.EXPORT' | translate }}\n      </button>\n    </div>\n  </div>\n</div>\n"

/***/ }),

/***/ "./src/app/export-import/export-import.component.scss":
/*!************************************************************!*\
  !*** ./src/app/export-import/export-import.component.scss ***!
  \************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.head {\n  justify-content: flex-end; }\n\n.contacts-title {\n  font-size: 1.7rem;\n  margin-bottom: 1rem; }\n\n.btn-wrapper {\n  display: flex;\n  align-items: center;\n  justify-content: space-between;\n  margin: 0 -0.5rem;\n  padding: 1.5rem 0; }\n\n.btn-wrapper button {\n    flex: 1 0 auto;\n    margin: 0 0.5rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9leHBvcnQtaW1wb3J0L0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxleHBvcnQtaW1wb3J0XFxleHBvcnQtaW1wb3J0LmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsV0FBVyxFQUFBOztBQUdiO0VBQ0UseUJBQXlCLEVBQUE7O0FBRzNCO0VBQ0UsaUJBQWlCO0VBQ2pCLG1CQUFtQixFQUFBOztBQUdyQjtFQUNFLGFBQWE7RUFDYixtQkFBbUI7RUFDbkIsOEJBQThCO0VBQzlCLGlCQUFpQjtFQUNqQixpQkFBaUIsRUFBQTs7QUFMbkI7SUFRSSxjQUFjO0lBQ2QsZ0JBQWdCLEVBQUEiLCJmaWxlIjoiYXBwL2V4cG9ydC1pbXBvcnQvZXhwb3J0LWltcG9ydC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICB3aWR0aDogMTAwJTtcclxufVxyXG5cclxuLmhlYWQge1xyXG4gIGp1c3RpZnktY29udGVudDogZmxleC1lbmQ7XHJcbn1cclxuXHJcbi5jb250YWN0cy10aXRsZSB7XHJcbiAgZm9udC1zaXplOiAxLjdyZW07XHJcbiAgbWFyZ2luLWJvdHRvbTogMXJlbTtcclxufVxyXG5cclxuLmJ0bi13cmFwcGVyIHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAganVzdGlmeS1jb250ZW50OiBzcGFjZS1iZXR3ZWVuO1xyXG4gIG1hcmdpbjogMCAtMC41cmVtO1xyXG4gIHBhZGRpbmc6IDEuNXJlbSAwO1xyXG5cclxuICBidXR0b24ge1xyXG4gICAgZmxleDogMSAwIGF1dG87XHJcbiAgICBtYXJnaW46IDAgMC41cmVtO1xyXG4gIH1cclxufSJdfQ== */"

/***/ }),

/***/ "./src/app/export-import/export-import.component.ts":
/*!**********************************************************!*\
  !*** ./src/app/export-import/export-import.component.ts ***!
  \**********************************************************/
/*! exports provided: ExportImportComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "ExportImportComponent", function() { return ExportImportComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_common__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/common */ "./node_modules/@angular/common/fesm5/common.js");
/* harmony import */ var _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../_helpers/services/backend.service */ "./src/app/_helpers/services/backend.service.ts");
/* harmony import */ var _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ../_helpers/services/variables.service */ "./src/app/_helpers/services/variables.service.ts");
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
/* harmony import */ var ngx_papaparse__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ngx-papaparse */ "./node_modules/ngx-papaparse/fesm5/ngx-papaparse.js");
/* harmony import */ var _ngx_translate_core__WEBPACK_IMPORTED_MODULE_6__ = __webpack_require__(/*! @ngx-translate/core */ "./node_modules/@ngx-translate/core/fesm5/ngx-translate-core.js");
var __decorate = (undefined && undefined.__decorate) || function (decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
};
var __metadata = (undefined && undefined.__metadata) || function (k, v) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
};







var ExportImportComponent = /** @class */ (function () {
    function ExportImportComponent(location, variablesService, backend, modalService, papa, translate) {
        this.location = location;
        this.variablesService = variablesService;
        this.backend = backend;
        this.modalService = modalService;
        this.papa = papa;
        this.translate = translate;
    }
    ExportImportComponent.prototype.ngOnInit = function () { };
    ExportImportComponent.prototype.import = function () {
        var _this = this;
        this.backend.openFileDialog('', '*', this.variablesService.settings.default_path, function (file_status, file_data) {
            if (file_status) {
                _this.variablesService.settings.default_path = file_data.path.substr(0, file_data.path.lastIndexOf('/'));
                if (_this.isValid(file_data.path)) {
                    _this.backend.loadFile(file_data.path, function (status, data) {
                        if (status) {
                            var options = {
                                header: true
                            };
                            var elements = _this.papa.parse(data, options);
                            if (elements.data && !elements.errors.length) {
                                if (!_this.variablesService.contacts.length) {
                                    elements.data.forEach(function (element) {
                                        _this.variablesService.contacts.push(element);
                                    });
                                }
                                else {
                                    elements.data.forEach(function (element) {
                                        var indexName = _this.variablesService.contacts.findIndex(function (contact) { return contact.name === element.name; });
                                        var indexAddress = _this.variablesService.contacts.findIndex(function (contact) { return contact.address === element.address; });
                                        if (indexAddress === -1 && indexName === -1) {
                                            _this.variablesService.contacts.push(element);
                                        }
                                        if (indexName !== -1 && indexAddress === -1) {
                                            _this.variablesService.contacts.push({
                                                name: element.name + " " + _this.translate.instant('CONTACTS.COPY'),
                                                address: element.address,
                                                notes: element.notes
                                            });
                                        }
                                    });
                                }
                                _this.backend.getContactAlias();
                                _this.modalService.prepareModal('success', 'CONTACTS.SUCCESS_IMPORT');
                            }
                            if (elements.errors.length) {
                                _this.modalService.prepareModal('error', 'CONTACTS.ERROR_IMPORT');
                                console.log(elements.errors);
                            }
                        }
                    });
                }
                else {
                    _this.modalService.prepareModal('error', 'CONTACTS.ERROR_TYPE_FILE');
                }
            }
        });
    };
    ExportImportComponent.prototype.export = function () {
        var _this = this;
        var contacts = [];
        this.variablesService.contacts.forEach(function (contact) {
            delete contact.alias;
            contacts.push(contact);
        });
        this.backend.saveFileDialog('', '*', this.variablesService.settings.default_path, function (file_status, file_data) {
            if (file_status) {
                _this.backend.storeFile(file_data.path, _this.papa.unparse(contacts));
            }
        });
    };
    ExportImportComponent.prototype.isValid = function (file) {
        return file.endsWith('.csv');
    };
    ExportImportComponent.prototype.back = function () {
        this.location.back();
    };
    ExportImportComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-export-import',
            template: __webpack_require__(/*! ./export-import.component.html */ "./src/app/export-import/export-import.component.html"),
            styles: [__webpack_require__(/*! ./export-import.component.scss */ "./src/app/export-import/export-import.component.scss")]
        }),
        __metadata("design:paramtypes", [_angular_common__WEBPACK_IMPORTED_MODULE_1__["Location"],
            _helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_3__["VariablesService"],
            _helpers_services_backend_service__WEBPACK_IMPORTED_MODULE_2__["BackendService"],
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__["ModalService"],
            ngx_papaparse__WEBPACK_IMPORTED_MODULE_5__["Papa"],
            _ngx_translate_core__WEBPACK_IMPORTED_MODULE_6__["TranslateService"]])
    ], ExportImportComponent);
    return ExportImportComponent;
}());



/***/ }),

/***/ "./src/app/history/history.component.html":
/*!************************************************!*\
  !*** ./src/app/history/history.component.html ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"wrap-table\">\r\n\r\n  <table class=\"history-table\">\r\n    <thead>\r\n    <tr #head (window:resize)=\"calculateWidth()\">\r\n      <th>{{ 'HISTORY.STATUS' | translate }}</th>\r\n      <th>{{ 'HISTORY.DATE' | translate }}</th>\r\n      <th>{{ 'HISTORY.AMOUNT' | translate }}</th>\r\n      <th>{{ 'HISTORY.FEE' | translate }}</th>\r\n      <th>{{ 'HISTORY.ADDRESS' | translate }}</th>\r\n    </tr>\r\n    </thead>\r\n    <tbody>\r\n    <ng-container *ngFor=\"let item of variablesService.currentWallet.history\">\r\n      <tr (click)=\"openDetails(item.tx_hash)\" [class.locked-transaction]=\"!item.is_mining && item.unlock_time > 0\">\r\n        <td> \r\n          <div class=\"status\" [class.send]=\"!item.is_income\" [class.received]=\"item.is_income\">\r\n            <ng-container *ngIf=\"variablesService.height_app - item.height < 10 || item.height === 0 && item.timestamp > 0\">\r\n              <div class=\"confirmation\" tooltip=\"{{ 'HISTORY.STATUS_TOOLTIP' | translate : {'current': getHeight(item)/10, 'total': 10} }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\">\r\n                <div class=\"fill\" [style.height]=\"getHeight(item) + '%'\"></div>\r\n              </div>\r\n            </ng-container>\r\n\r\n            <ng-container *ngIf=\"item.unlock_time !== 0 && item.tx_type !== 6\">\r\n              <ng-container *ngIf=\"isLocked(item); else unlock\">\r\n                <ng-container *ngIf=\"item.unlock_time < 500000000\">\r\n                  <i class=\"icon lock-transaction\" tooltip=\"{{ 'HISTORY.LOCK_TOOLTIP' | translate : {'date': time(item) | date : 'MM.dd.yy'} }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\"\r\n                  [class.position]=\"variablesService.height_app - item.height < 10 || item.height === 0 && item.timestamp > 0\"></i>\r\n                </ng-container>\r\n                <ng-container *ngIf=\"item.unlock_time > 500000000\">\r\n                  <i class=\"icon lock-transaction\" tooltip=\"{{ 'HISTORY.LOCK_TOOLTIP' | translate : {'date': item.unlock_time * 1000 | date : 'MM.dd.yy'} }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\"\r\n                  [class.position]=\"variablesService.height_app - item.height < 10 || item.height === 0 && item.timestamp > 0\"></i>\r\n                </ng-container>\r\n              </ng-container> \r\n              <ng-template #unlock>\r\n                <i class=\"icon unlock-transaction\" placement=\"bottom-left\" [class.position]=\"variablesService.height_app - item.height < 10 || item.height === 0 && item.timestamp > 0\"></i>\r\n              </ng-template>\r\n            </ng-container>\r\n\r\n            <!-- <ng-container *ngIf=\"!item.is_mining && item.unlock_time > 0\">\r\n              <i class=\"icon lock-transaction\" tooltip=\"{{ 'HISTORY.LOCK_TOOLTIP' | translate : {'date': item.unlock_time * 1000 | date : 'MM.dd.yy'} }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\"></i>\r\n            </ng-container> -->\r\n            <i class=\"icon status-transaction\"></i>\r\n            <span>{{ (item.is_income ? 'HISTORY.RECEIVED' : 'HISTORY.SEND') | translate }}</span>\r\n          </div>\r\n        </td>\r\n        <td>{{item.timestamp * 1000 | date : 'dd-MM-yyyy HH:mm'}}</td>\r\n        <td>\r\n          <span *ngIf=\"item.sortAmount && item.sortAmount.toString() !== '0'\">{{item.sortAmount | intToMoney}} {{variablesService.defaultCurrency}}</span>\r\n        </td>\r\n        <td>\r\n          <span *ngIf=\"item.sortFee && item.sortFee.toString() !== '0'\">{{item.sortFee | intToMoney}} {{variablesService.defaultCurrency}}</span>\r\n        </td>\r\n        <td class=\"remote-address\">\r\n          <span *ngIf=\"!(item.tx_type === 0 && item.remote_addresses && item.remote_addresses[0])\">{{item | historyTypeMessages}}</span>\r\n          <span *ngIf=\"item.tx_type === 0 && item.remote_addresses && item.remote_addresses[0]\" (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, item.remote_addresses[0])\">{{item.remote_addresses[0]}}</span>\r\n        </td>\r\n      </tr>\r\n      <tr class=\"transaction-details\" [class.open]=\"item.tx_hash === openedDetails\">\r\n        <td colspan=\"5\">\r\n          <ng-container *ngIf=\"item.tx_hash === openedDetails\">\r\n            <app-transaction-details [transaction]=\"item\" [sizes]=\"calculatedWidth\"></app-transaction-details>\r\n          </ng-container>\r\n        </td>\r\n      </tr>\r\n    </ng-container>\r\n    </tbody>\r\n  </table>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/history/history.component.scss":
/*!************************************************!*\
  !*** ./src/app/history/history.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.wrap-table {\n  margin: -3rem; }\n\n.wrap-table table tbody tr td {\n    min-width: 10rem; }\n\n.wrap-table table tbody tr .status {\n    position: relative;\n    display: flex;\n    align-items: center; }\n\n.wrap-table table tbody tr .status .confirmation {\n      position: absolute;\n      top: 50%;\n      left: -2rem;\n      transform: translateY(-50%);\n      display: flex;\n      align-items: flex-end;\n      width: 0.7rem;\n      height: 1.5rem; }\n\n.wrap-table table tbody tr .status .confirmation .fill {\n        width: 100%; }\n\n.wrap-table table tbody tr .status .lock-transaction {\n      position: absolute;\n      left: -2rem;\n      -webkit-mask: url('lock-transaction.svg') no-repeat center;\n              mask: url('lock-transaction.svg') no-repeat center;\n      width: 1.2rem;\n      height: 1.2rem;\n      margin-right: 1.1rem; }\n\n.wrap-table table tbody tr .status .unlock-transaction {\n      position: absolute;\n      left: -2rem;\n      -webkit-mask: url('unlock-transaction.svg') no-repeat center;\n              mask: url('unlock-transaction.svg') no-repeat center;\n      width: 1.2rem;\n      height: 1.2rem;\n      margin-right: 1.1rem; }\n\n.wrap-table table tbody tr .status .position {\n      position: static; }\n\n.wrap-table table tbody tr .status .status-transaction {\n      margin-right: 1rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.wrap-table table tbody tr .status.send .status-transaction {\n      -webkit-mask: url('send.svg') no-repeat center;\n              mask: url('send.svg') no-repeat center; }\n\n.wrap-table table tbody tr .status.received .status-transaction {\n      -webkit-mask: url('receive.svg') no-repeat center;\n              mask: url('receive.svg') no-repeat center; }\n\n.wrap-table table tbody tr .remote-address {\n    overflow: hidden;\n    text-overflow: ellipsis;\n    max-width: 25vw; }\n\n.wrap-table table tbody tr:not(.transaction-details) {\n    cursor: pointer; }\n\n.wrap-table table tbody tr.transaction-details {\n    transition: 0.5s height linear, 0s font-size;\n    transition-delay: 0s, 0.5s;\n    height: 0; }\n\n.wrap-table table tbody tr.transaction-details.open {\n      height: 16.2rem; }\n\n.wrap-table table tbody tr.transaction-details td {\n      position: relative;\n      overflow: hidden;\n      line-height: inherit;\n      padding-top: 0;\n      padding-bottom: 0; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9oaXN0b3J5L0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxoaXN0b3J5XFxoaXN0b3J5LmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsV0FBVyxFQUFBOztBQUdiO0VBQ0UsYUFBYSxFQUFBOztBQURmO0lBVVUsZ0JBQWdCLEVBQUE7O0FBVjFCO0lBY1Usa0JBQWtCO0lBQ2xCLGFBQWE7SUFDYixtQkFBbUIsRUFBQTs7QUFoQjdCO01BbUJZLGtCQUFrQjtNQUNsQixRQUFRO01BQ1IsV0FBVztNQUNYLDJCQUEyQjtNQUMzQixhQUFhO01BQ2IscUJBQXFCO01BQ3JCLGFBQWE7TUFDYixjQUFjLEVBQUE7O0FBMUIxQjtRQTZCYyxXQUFXLEVBQUE7O0FBN0J6QjtNQWtDWSxrQkFBa0I7TUFDbEIsV0FBVztNQUNYLDBEQUFtRTtjQUFuRSxrREFBbUU7TUFDbkUsYUFBYTtNQUNiLGNBQWM7TUFDZCxvQkFBb0IsRUFBQTs7QUF2Q2hDO01BMkNZLGtCQUFrQjtNQUNsQixXQUFXO01BQ1gsNERBQXFFO2NBQXJFLG9EQUFxRTtNQUNyRSxhQUFhO01BQ2IsY0FBYztNQUNkLG9CQUFvQixFQUFBOztBQWhEaEM7TUFvRFksZ0JBQWdCLEVBQUE7O0FBcEQ1QjtNQXdEWSxrQkFBa0I7TUFDbEIsYUFBYTtNQUNiLGNBQWMsRUFBQTs7QUExRDFCO01BZ0VjLDhDQUF1RDtjQUF2RCxzQ0FBdUQsRUFBQTs7QUFoRXJFO01BdUVjLGlEQUEwRDtjQUExRCx5Q0FBMEQsRUFBQTs7QUF2RXhFO0lBNkVVLGdCQUFnQjtJQUNoQix1QkFBdUI7SUFDdkIsZUFBZSxFQUFBOztBQS9FekI7SUFtRlUsZUFBZSxFQUFBOztBQW5GekI7SUF3RlUsNENBQTRDO0lBQzVDLDBCQUEwQjtJQUMxQixTQUFTLEVBQUE7O0FBMUZuQjtNQTZGWSxlQUFlLEVBQUE7O0FBN0YzQjtNQWlHWSxrQkFBa0I7TUFDbEIsZ0JBQWdCO01BQ2hCLG9CQUFvQjtNQUNwQixjQUFjO01BQ2QsaUJBQWlCLEVBQUEiLCJmaWxlIjoiYXBwL2hpc3RvcnkvaGlzdG9yeS5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICB3aWR0aDogMTAwJTtcclxufVxyXG5cclxuLndyYXAtdGFibGUge1xyXG4gIG1hcmdpbjogLTNyZW07XHJcblxyXG4gIHRhYmxlIHtcclxuXHJcbiAgICB0Ym9keSB7XHJcblxyXG4gICAgICB0ciB7XHJcblxyXG4gICAgICAgIHRkIHtcclxuICAgICAgICAgIG1pbi13aWR0aDogMTByZW07XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAuc3RhdHVzIHtcclxuICAgICAgICAgIHBvc2l0aW9uOiByZWxhdGl2ZTtcclxuICAgICAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG5cclxuICAgICAgICAgIC5jb25maXJtYXRpb24ge1xyXG4gICAgICAgICAgICBwb3NpdGlvbjogYWJzb2x1dGU7XHJcbiAgICAgICAgICAgIHRvcDogNTAlO1xyXG4gICAgICAgICAgICBsZWZ0OiAtMnJlbTtcclxuICAgICAgICAgICAgdHJhbnNmb3JtOiB0cmFuc2xhdGVZKC01MCUpO1xyXG4gICAgICAgICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICAgICAgICBhbGlnbi1pdGVtczogZmxleC1lbmQ7XHJcbiAgICAgICAgICAgIHdpZHRoOiAwLjdyZW07XHJcbiAgICAgICAgICAgIGhlaWdodDogMS41cmVtO1xyXG5cclxuICAgICAgICAgICAgLmZpbGwge1xyXG4gICAgICAgICAgICAgIHdpZHRoOiAxMDAlO1xyXG4gICAgICAgICAgICB9XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgLmxvY2stdHJhbnNhY3Rpb24ge1xyXG4gICAgICAgICAgICBwb3NpdGlvbjogYWJzb2x1dGU7XHJcbiAgICAgICAgICAgIGxlZnQ6IC0ycmVtO1xyXG4gICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2xvY2stdHJhbnNhY3Rpb24uc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgICAgICB3aWR0aDogMS4ycmVtO1xyXG4gICAgICAgICAgICBoZWlnaHQ6IDEuMnJlbTtcclxuICAgICAgICAgICAgbWFyZ2luLXJpZ2h0OiAxLjFyZW07XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgLnVubG9jay10cmFuc2FjdGlvbiB7XHJcbiAgICAgICAgICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuICAgICAgICAgICAgbGVmdDogLTJyZW07XHJcbiAgICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvdW5sb2NrLXRyYW5zYWN0aW9uLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICAgICAgd2lkdGg6IDEuMnJlbTtcclxuICAgICAgICAgICAgaGVpZ2h0OiAxLjJyZW07XHJcbiAgICAgICAgICAgIG1hcmdpbi1yaWdodDogMS4xcmVtO1xyXG4gICAgICAgICAgfVxyXG5cclxuICAgICAgICAgIC5wb3NpdGlvbiB7XHJcbiAgICAgICAgICAgIHBvc2l0aW9uOiBzdGF0aWM7XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgLnN0YXR1cy10cmFuc2FjdGlvbiB7XHJcbiAgICAgICAgICAgIG1hcmdpbi1yaWdodDogMXJlbTtcclxuICAgICAgICAgICAgd2lkdGg6IDEuN3JlbTtcclxuICAgICAgICAgICAgaGVpZ2h0OiAxLjdyZW07XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgJi5zZW5kICB7XHJcblxyXG4gICAgICAgICAgICAuc3RhdHVzLXRyYW5zYWN0aW9uIHtcclxuICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3NlbmQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgICAgICB9XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgJi5yZWNlaXZlZCB7XHJcblxyXG4gICAgICAgICAgICAuc3RhdHVzLXRyYW5zYWN0aW9uIHtcclxuICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3JlY2VpdmUuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgICAgICB9XHJcbiAgICAgICAgICB9XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAucmVtb3RlLWFkZHJlc3Mge1xyXG4gICAgICAgICAgb3ZlcmZsb3c6IGhpZGRlbjtcclxuICAgICAgICAgIHRleHQtb3ZlcmZsb3c6IGVsbGlwc2lzO1xyXG4gICAgICAgICAgbWF4LXdpZHRoOiAyNXZ3O1xyXG4gICAgICAgIH1cclxuXHJcbiAgICAgICAgJjpub3QoLnRyYW5zYWN0aW9uLWRldGFpbHMpIHtcclxuICAgICAgICAgIGN1cnNvcjogcG9pbnRlcjtcclxuICAgICAgICB9XHJcblxyXG4gICAgICAgICYudHJhbnNhY3Rpb24tZGV0YWlscyB7XHJcbiAgICAgICAgICAtd2Via2l0LXRyYW5zaXRpb246IDAuNXMgaGVpZ2h0IGxpbmVhciwgMHMgZm9udC1zaXplO1xyXG4gICAgICAgICAgdHJhbnNpdGlvbjogMC41cyBoZWlnaHQgbGluZWFyLCAwcyBmb250LXNpemU7XHJcbiAgICAgICAgICB0cmFuc2l0aW9uLWRlbGF5OiAwcywgMC41cztcclxuICAgICAgICAgIGhlaWdodDogMDtcclxuXHJcbiAgICAgICAgICAmLm9wZW4ge1xyXG4gICAgICAgICAgICBoZWlnaHQ6IDE2LjJyZW07XHJcbiAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgdGQge1xyXG4gICAgICAgICAgICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XHJcbiAgICAgICAgICAgIGxpbmUtaGVpZ2h0OiBpbmhlcml0O1xyXG4gICAgICAgICAgICBwYWRkaW5nLXRvcDogMDtcclxuICAgICAgICAgICAgcGFkZGluZy1ib3R0b206IDA7XHJcbiAgICAgICAgICB9XHJcbiAgICAgICAgfVxyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcbiJdfQ== */"

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
    HistoryComponent.prototype.time = function (item) {
        var now = new Date().getTime();
        var unlockTime = now + ((item.unlock_time - this.variablesService.height_app) * 60 * 1000);
        return unlockTime;
    };
    HistoryComponent.prototype.isLocked = function (item) {
        if ((item.unlock_time > 500000000) && (item.unlock_time > new Date().getTime() / 1000)) {
            console.log(new Date().getTime());
            return true;
        }
        if ((item.unlock_time < 500000000) && (item.unlock_time > this.variablesService.height_app)) {
            return true;
        }
        return false;
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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"wrap-login\">\r\n\r\n    <div class=\"logo\"></div>\r\n\r\n    <form *ngIf=\"type === 'reg'\" class=\"form-login\" [formGroup]=\"regForm\" (ngSubmit)=\"onSubmitCreatePass()\">\r\n\r\n      <div class=\"input-block\">\r\n        <label for=\"master-pass\">{{ 'LOGIN.SETUP_MASTER_PASS' | translate }}</label>\r\n        <input type=\"password\" id=\"master-pass\" formControlName=\"password\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\r\n      </div>\r\n\r\n      <div class=\"input-block\">\r\n        <label for=\"confirm-pass\">{{ 'LOGIN.SETUP_CONFIRM_PASS' | translate }}</label>\r\n        <input type=\"password\" id=\"confirm-pass\" formControlName=\"confirmation\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\r\n        <div class=\"error-block\" *ngIf=\"regForm.controls['password'].dirty && regForm.controls['confirmation'].dirty && regForm.errors\">\r\n          <div *ngIf=\"regForm.errors['mismatch']\">\r\n            {{ 'LOGIN.FORM_ERRORS.MISMATCH' | translate }}\r\n          </div>\r\n        </div>\r\n      </div>\r\n\r\n      <div class=\"wrap-button\">\r\n        <button type=\"submit\" class=\"blue-button\" [disabled]=\"!regForm.controls['password'].value.length || !regForm.controls['confirmation'].value.length || (regForm.errors && regForm.errors['mismatch'])\">{{ 'LOGIN.BUTTON_NEXT' | translate }}</button>\r\n        <button type=\"button\" class=\"blue-button\" (click)=\"onSkipCreatePass()\" [disabled]=\"regForm.controls['password'].value.length || regForm.controls['confirmation'].value.length\">{{ 'LOGIN.BUTTON_SKIP' | translate }}</button>\r\n      </div>\r\n\r\n    </form>\r\n\r\n    <form *ngIf=\"type !== 'reg'\" class=\"form-login\" [formGroup]=\"authForm\" (ngSubmit)=\"onSubmitAuthPass()\">\r\n\r\n      <div class=\"input-block\">\r\n        <label for=\"master-pass-login\">{{ 'LOGIN.MASTER_PASS' | translate }}</label>\r\n        <input type=\"password\" id=\"master-pass-login\" formControlName=\"password\" autofocus (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\r\n      </div>\r\n\r\n      <div class=\"wrap-button\">\r\n        <button type=\"submit\" class=\"blue-button\">{{ 'LOGIN.BUTTON_NEXT' | translate }}</button>\r\n        <button type=\"button\" class=\"blue-button_reset\" (click)=\"dropSecureAppData()\">{{ 'LOGIN.BUTTON_RESET' | translate }}</button>   <!--Add \"Reset\"-button-->\r\n      </div> \r\n\r\n    </form>\r\n\r\n  </div>\r\n\r\n</div>"

/***/ }),

/***/ "./src/app/login/login.component.scss":
/*!********************************************!*\
  !*** ./src/app/login/login.component.scss ***!
  \********************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: fixed;\n  top: 0;\n  left: 0;\n  width: 100%;\n  height: 100%; }\n  :host .content {\n    display: flex; }\n  :host .content .wrap-login {\n      margin: auto;\n      width: 100%;\n      max-width: 40rem; }\n  :host .content .wrap-login .logo {\n        background: url('logo.svg') no-repeat center;\n        width: 100%;\n        height: 15rem; }\n  :host .content .wrap-login .form-login {\n        display: flex;\n        flex-direction: column; }\n  :host .content .wrap-login .form-login .wrap-button {\n          display: flex;\n          align-items: center;\n          justify-content: space-between; }\n  :host .content .wrap-login .form-login .wrap-button button {\n            margin: 2.5rem 0; }\n  :host .content .wrap-login .form-login button {\n          margin: 2.5rem auto;\n          width: 100%;\n          max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9sb2dpbi9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcbG9naW5cXGxvZ2luLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZUFBZTtFQUNmLE1BQU07RUFDTixPQUFPO0VBQ1AsV0FBVztFQUNYLFlBQVksRUFBQTtFQUxkO0lBUUksYUFBYSxFQUFBO0VBUmpCO01BV00sWUFBWTtNQUNaLFdBQVc7TUFDWCxnQkFBZ0IsRUFBQTtFQWJ0QjtRQWdCUSw0Q0FBNkQ7UUFDN0QsV0FBVztRQUNYLGFBQWEsRUFBQTtFQWxCckI7UUFzQlEsYUFBYTtRQUNiLHNCQUFzQixFQUFBO0VBdkI5QjtVQTBCVSxhQUFhO1VBQ2IsbUJBQW1CO1VBQ25CLDhCQUE4QixFQUFBO0VBNUJ4QztZQStCWSxnQkFBZ0IsRUFBQTtFQS9CNUI7VUFvQ1UsbUJBQW1CO1VBQ25CLFdBQVc7VUFDWCxnQkFBZ0IsRUFBQSIsImZpbGUiOiJhcHAvbG9naW4vbG9naW4uY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XHJcbiAgcG9zaXRpb246IGZpeGVkO1xyXG4gIHRvcDogMDtcclxuICBsZWZ0OiAwO1xyXG4gIHdpZHRoOiAxMDAlO1xyXG4gIGhlaWdodDogMTAwJTtcclxuXHJcbiAgLmNvbnRlbnQge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuXHJcbiAgICAud3JhcC1sb2dpbiB7XHJcbiAgICAgIG1hcmdpbjogYXV0bztcclxuICAgICAgd2lkdGg6IDEwMCU7XHJcbiAgICAgIG1heC13aWR0aDogNDByZW07XHJcblxyXG4gICAgICAubG9nbyB7XHJcbiAgICAgICAgYmFja2dyb3VuZDogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9sb2dvLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICB3aWR0aDogMTAwJTtcclxuICAgICAgICBoZWlnaHQ6IDE1cmVtO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAuZm9ybS1sb2dpbiB7XHJcbiAgICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG5cclxuICAgICAgICAud3JhcC1idXR0b24ge1xyXG4gICAgICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICAgICAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcblxyXG4gICAgICAgICAgYnV0dG9uIHtcclxuICAgICAgICAgICAgbWFyZ2luOiAyLjVyZW0gMDtcclxuICAgICAgICAgIH1cclxuICAgICAgICB9XHJcblxyXG4gICAgICAgIGJ1dHRvbiB7XHJcbiAgICAgICAgICBtYXJnaW46IDIuNXJlbSBhdXRvO1xyXG4gICAgICAgICAgd2lkdGg6IDEwMCU7XHJcbiAgICAgICAgICBtYXgtd2lkdGg6IDE1cmVtO1xyXG4gICAgICAgIH1cclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

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
            this.variablesService.appPass = this.regForm.get('password').value; //the pass what was written in input of login form by user
            this.backend.setMasterPassword({ pass: this.variablesService.appPass }, function (status, data) {
                if (status) {
                    _this.backend.storeSecureAppData({ pass: _this.variablesService.appPass });
                    _this.variablesService.appLogin = true;
                    _this.variablesService.dataIsLoaded = true;
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
    LoginComponent.prototype.dropSecureAppData = function () {
        var _this = this;
        this.backend.dropSecureAppData(function () {
            _this.onSkipCreatePass();
        });
        this.variablesService.wallets = [];
        this.variablesService.contacts = [];
    };
    LoginComponent.prototype.onSubmitAuthPass = function () {
        var _this = this;
        if (this.authForm.valid) {
            this.variablesService.appPass = this.authForm.get('password').value;
            if (this.variablesService.dataIsLoaded) {
                this.backend.checkMasterPassword({ pass: this.variablesService.appPass }, function (status, data) {
                    if (status) {
                        _this.variablesService.appLogin = true;
                        _this.variablesService.startCountdown();
                        _this.ngZone.run(function () {
                            _this.router.navigate(['/']);
                        });
                    }
                });
            }
            else {
                this.getWalletData(this.variablesService.appPass);
            }
        }
    };
    LoginComponent.prototype.getWalletData = function (appPass) {
        var _this = this;
        this.backend.getSecureAppData({ pass: appPass }, function (status, data) {
            if (!data.error_code) {
                _this.variablesService.appLogin = true;
                _this.variablesService.dataIsLoaded = true;
                _this.variablesService.startCountdown();
                _this.variablesService.appPass = appPass;
                if (Object.keys(data['contacts']).length !== 0) {
                    data['contacts'].map(function (contact) {
                        _this.variablesService.contacts.push(contact);
                    });
                }
                if (_this.variablesService.wallets.length) {
                    _this.ngZone.run(function () {
                        _this.router.navigate(['/wallet/' + _this.variablesService.wallets[0].wallet_id]);
                    });
                    return;
                }
                if (Object.keys(data['wallets']).length !== 0) {
                    var openWallets_1 = 0;
                    var runWallets_1 = 0;
                    data['wallets'].forEach(function (wallet, wallet_index) {
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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\" *ngIf=\"variablesService.wallets.length > 0\">\r\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <div class=\"add-wallet\">\r\n    <h3 class=\"add-wallet-title\">{{ 'MAIN.TITLE' | translate }}</h3>\r\n    <div class=\"add-wallet-buttons\">\r\n      <button type=\"button\" class=\"blue-button\" [routerLink]=\"['/create']\">{{ 'MAIN.BUTTON_NEW_WALLET' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"openWallet()\">{{ 'MAIN.BUTTON_OPEN_WALLET' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button\" [routerLink]=\"['/restore']\">{{ 'MAIN.BUTTON_RESTORE_BACKUP' | translate }}</button>\r\n    </div>\r\n    <div class=\"add-wallet-help\" (click)=\"openInBrowser()\">\r\n      <i class=\"icon\"></i><span>{{ 'MAIN.HELP' | translate }}</span>\r\n    </div>\r\n  </div>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/main/main.component.scss":
/*!******************************************!*\
  !*** ./src/app/main/main.component.scss ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  flex: 1 0 auto;\n  padding: 3rem; }\n\n.content {\n  padding: 3rem;\n  min-height: 100%; }\n\n.content .head {\n    justify-content: flex-end; }\n\n.add-wallet .add-wallet-title {\n  margin-bottom: 1rem; }\n\n.add-wallet .add-wallet-buttons {\n  display: flex;\n  align-items: center;\n  justify-content: space-between;\n  margin: 0 -0.5rem;\n  padding: 1.5rem 0; }\n\n.add-wallet .add-wallet-buttons button {\n    flex: 1 0 auto;\n    margin: 0 0.5rem; }\n\n.add-wallet .add-wallet-help {\n  display: flex;\n  cursor: pointer;\n  font-size: 1.3rem;\n  line-height: 1.4rem; }\n\n.add-wallet .add-wallet-help .icon {\n    -webkit-mask: url('howto.svg') no-repeat center;\n            mask: url('howto.svg') no-repeat center;\n    margin-right: 0.8rem;\n    width: 1.4rem;\n    height: 1.4rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9tYWluL0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxtYWluXFxtYWluLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsY0FBYztFQUNkLGFBQWEsRUFBQTs7QUFHZjtFQUNFLGFBQWE7RUFDYixnQkFBZ0IsRUFBQTs7QUFGbEI7SUFLSSx5QkFBeUIsRUFBQTs7QUFJN0I7RUFHSSxtQkFBbUIsRUFBQTs7QUFIdkI7RUFPSSxhQUFhO0VBQ2IsbUJBQW1CO0VBQ25CLDhCQUE4QjtFQUM5QixpQkFBaUI7RUFDakIsaUJBQWlCLEVBQUE7O0FBWHJCO0lBY00sY0FBYztJQUNkLGdCQUFnQixFQUFBOztBQWZ0QjtFQW9CSSxhQUFhO0VBQ2IsZUFBZTtFQUNmLGlCQUFpQjtFQUNqQixtQkFBbUIsRUFBQTs7QUF2QnZCO0lBMEJNLCtDQUF3RDtZQUF4RCx1Q0FBd0Q7SUFDeEQsb0JBQW9CO0lBQ3BCLGFBQWE7SUFDYixjQUFjLEVBQUEiLCJmaWxlIjoiYXBwL21haW4vbWFpbi5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBmbGV4OiAxIDAgYXV0bztcclxuICBwYWRkaW5nOiAzcmVtO1xyXG59XHJcblxyXG4uY29udGVudCB7XHJcbiAgcGFkZGluZzogM3JlbTtcclxuICBtaW4taGVpZ2h0OiAxMDAlO1xyXG5cclxuICAuaGVhZCB7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGZsZXgtZW5kO1xyXG4gIH1cclxufVxyXG5cclxuLmFkZC13YWxsZXQge1xyXG5cclxuICAuYWRkLXdhbGxldC10aXRsZSB7XHJcbiAgICBtYXJnaW4tYm90dG9tOiAxcmVtO1xyXG4gIH1cclxuXHJcbiAgLmFkZC13YWxsZXQtYnV0dG9ucyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICAgIG1hcmdpbjogMCAtMC41cmVtO1xyXG4gICAgcGFkZGluZzogMS41cmVtIDA7XHJcblxyXG4gICAgYnV0dG9uIHtcclxuICAgICAgZmxleDogMSAwIGF1dG87XHJcbiAgICAgIG1hcmdpbjogMCAwLjVyZW07XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAuYWRkLXdhbGxldC1oZWxwIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBjdXJzb3I6IHBvaW50ZXI7XHJcbiAgICBmb250LXNpemU6IDEuM3JlbTtcclxuICAgIGxpbmUtaGVpZ2h0OiAxLjRyZW07XHJcblxyXG4gICAgLmljb24ge1xyXG4gICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2hvd3RvLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgbWFyZ2luLXJpZ2h0OiAwLjhyZW07XHJcbiAgICAgIHdpZHRoOiAxLjRyZW07XHJcbiAgICAgIGhlaWdodDogMS40cmVtO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

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

module.exports = "<div class=\"wrap-table\">\r\n\r\n  <table class=\"messages-table\">\r\n    <thead>\r\n    <tr>\r\n      <th>{{ 'MESSAGES.ADDRESS' | translate }}</th>\r\n      <th>{{ 'MESSAGES.MESSAGE' | translate }}</th>\r\n    </tr>\r\n    </thead>\r\n    <tbody>\r\n    <tr *ngFor=\"let message of messages\" [routerLink]=\"[message.address]\">\r\n      <td>\r\n        <span>{{message.address}}</span>\r\n        <i class=\"icon\" *ngIf=\"message.is_new\"></i>\r\n      </td>\r\n      <td>\r\n        <span>{{message.message}}</span>\r\n      </td>\r\n    </tr>\r\n    </tbody>\r\n  </table>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/messages/messages.component.scss":
/*!**************************************************!*\
  !*** ./src/app/messages/messages.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.wrap-table {\n  margin: -3rem; }\n\n.wrap-table table tbody tr td:first-child {\n    position: relative;\n    padding-right: 5rem;\n    width: 18rem; }\n\n.wrap-table table tbody tr td:first-child span {\n      display: block;\n      line-height: 3.5rem;\n      max-width: 10rem; }\n\n.wrap-table table tbody tr td:first-child .icon {\n      position: absolute;\n      top: 50%;\n      right: 1rem;\n      transform: translateY(-50%);\n      display: block;\n      -webkit-mask: url('alert.svg') no-repeat 0;\n              mask: url('alert.svg') no-repeat 0;\n      width: 1.2rem;\n      height: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9tZXNzYWdlcy9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcbWVzc2FnZXNcXG1lc3NhZ2VzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsV0FBVyxFQUFBOztBQUdiO0VBQ0UsYUFBYSxFQUFBOztBQURmO0lBWVksa0JBQWtCO0lBQ2xCLG1CQUFtQjtJQUNuQixZQUFZLEVBQUE7O0FBZHhCO01BaUJjLGNBQWM7TUFDZCxtQkFBbUI7TUFDbkIsZ0JBQWdCLEVBQUE7O0FBbkI5QjtNQXVCYyxrQkFBa0I7TUFDbEIsUUFBUTtNQUNSLFdBQVc7TUFDWCwyQkFBMkI7TUFDM0IsY0FBYztNQUNkLDBDQUFtRDtjQUFuRCxrQ0FBbUQ7TUFDbkQsYUFBYTtNQUNiLGNBQWMsRUFBQSIsImZpbGUiOiJhcHAvbWVzc2FnZXMvbWVzc2FnZXMuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XHJcbiAgd2lkdGg6IDEwMCU7XHJcbn1cclxuXHJcbi53cmFwLXRhYmxlIHtcclxuICBtYXJnaW46IC0zcmVtO1xyXG5cclxuICB0YWJsZSB7XHJcblxyXG4gICAgdGJvZHkge1xyXG5cclxuICAgICAgdHIge1xyXG5cclxuICAgICAgICB0ZCB7XHJcblxyXG4gICAgICAgICAgJjpmaXJzdC1jaGlsZCB7XHJcbiAgICAgICAgICAgIHBvc2l0aW9uOiByZWxhdGl2ZTtcclxuICAgICAgICAgICAgcGFkZGluZy1yaWdodDogNXJlbTtcclxuICAgICAgICAgICAgd2lkdGg6IDE4cmVtO1xyXG5cclxuICAgICAgICAgICAgc3BhbiB7XHJcbiAgICAgICAgICAgICAgZGlzcGxheTogYmxvY2s7XHJcbiAgICAgICAgICAgICAgbGluZS1oZWlnaHQ6IDMuNXJlbTtcclxuICAgICAgICAgICAgICBtYXgtd2lkdGg6IDEwcmVtO1xyXG4gICAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgICAuaWNvbiB7XHJcbiAgICAgICAgICAgICAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gICAgICAgICAgICAgIHRvcDogNTAlO1xyXG4gICAgICAgICAgICAgIHJpZ2h0OiAxcmVtO1xyXG4gICAgICAgICAgICAgIHRyYW5zZm9ybTogdHJhbnNsYXRlWSgtNTAlKTtcclxuICAgICAgICAgICAgICBkaXNwbGF5OiBibG9jaztcclxuICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2FsZXJ0LnN2Zykgbm8tcmVwZWF0IDA7XHJcbiAgICAgICAgICAgICAgd2lkdGg6IDEuMnJlbTtcclxuICAgICAgICAgICAgICBoZWlnaHQ6IDEuMnJlbTtcclxuICAgICAgICAgICAgfVxyXG4gICAgICAgICAgfVxyXG4gICAgICAgIH1cclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

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

module.exports = "<div class=\"modal\">\r\n  <h3 class=\"title\">{{ 'OPEN_WALLET.MODAL.TITLE' | translate }}</h3>\r\n  <form class=\"open-form\" (ngSubmit)=\"openWallet()\">\r\n    <div class=\"wallet-path\">{{ wallet.name }}</div>\r\n    <div class=\"wallet-path\">{{ wallet.path }}</div>\r\n    <div class=\"input-block\" *ngIf=\"!wallet.notFound && !wallet.emptyPass\">\r\n      <label for=\"password\">{{ 'OPEN_WALLET.MODAL.LABEL' | translate }}</label>\r\n      <input type=\"password\" id=\"password\" name=\"password\" [(ngModel)]=\"wallet.pass\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\"/>\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"wallet.notFound\">\r\n      {{ 'OPEN_WALLET.MODAL.NOT_FOUND' | translate }}\r\n    </div>\r\n    <div class=\"wrap-button\">\r\n      <button type=\"submit\" class=\"blue-button\" [disabled]=\"wallet.notFound\">{{ 'OPEN_WALLET.MODAL.OPEN' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"skipWallet()\">{{ 'OPEN_WALLET.MODAL.SKIP' | translate }}</button>\r\n    </div>\r\n  </form>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/open-wallet-modal/open-wallet-modal.component.scss":
/*!********************************************************************!*\
  !*** ./src/app/open-wallet-modal/open-wallet-modal.component.scss ***!
  \********************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: fixed;\n  top: 0;\n  bottom: 0;\n  left: 0;\n  right: 0;\n  display: flex;\n  align-items: center;\n  justify-content: center;\n  background: rgba(255, 255, 255, 0.25); }\n\n.modal {\n  display: flex;\n  flex-direction: column;\n  background-position: center;\n  background-size: 200%;\n  padding: 2rem;\n  width: 34rem; }\n\n.modal .title {\n    font-size: 1.8rem;\n    text-align: center; }\n\n.modal .open-form .wallet-path {\n    font-size: 1.3rem;\n    margin: 5rem 0 2rem; }\n\n.modal .open-form .wrap-button {\n    display: flex;\n    align-items: center;\n    justify-content: space-between;\n    margin: 2rem -2rem 0; }\n\n.modal .open-form .wrap-button button {\n      flex: 1 0 0;\n      margin: 0 2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9vcGVuLXdhbGxldC1tb2RhbC9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcb3Blbi13YWxsZXQtbW9kYWxcXG9wZW4td2FsbGV0LW1vZGFsLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZUFBZTtFQUNmLE1BQU07RUFDTixTQUFTO0VBQ1QsT0FBTztFQUNQLFFBQVE7RUFDUixhQUFhO0VBQ2IsbUJBQW1CO0VBQ25CLHVCQUF1QjtFQUN2QixxQ0FBcUMsRUFBQTs7QUFHdkM7RUFDRSxhQUFhO0VBQ2Isc0JBQXNCO0VBQ3RCLDJCQUEyQjtFQUMzQixxQkFBcUI7RUFDckIsYUFBYTtFQUNiLFlBQVksRUFBQTs7QUFOZDtJQVNJLGlCQUFpQjtJQUNqQixrQkFBa0IsRUFBQTs7QUFWdEI7SUFnQk0saUJBQWlCO0lBQ2pCLG1CQUFtQixFQUFBOztBQWpCekI7SUFxQk0sYUFBYTtJQUNiLG1CQUFtQjtJQUNuQiw4QkFBOEI7SUFDOUIsb0JBQW9CLEVBQUE7O0FBeEIxQjtNQTJCUSxXQUFXO01BQ1gsY0FBZSxFQUFBIiwiZmlsZSI6ImFwcC9vcGVuLXdhbGxldC1tb2RhbC9vcGVuLXdhbGxldC1tb2RhbC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBwb3NpdGlvbjogZml4ZWQ7XHJcbiAgdG9wOiAwO1xyXG4gIGJvdHRvbTogMDtcclxuICBsZWZ0OiAwO1xyXG4gIHJpZ2h0OiAwO1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICBiYWNrZ3JvdW5kOiByZ2JhKDI1NSwgMjU1LCAyNTUsIDAuMjUpO1xyXG59XHJcblxyXG4ubW9kYWwge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICBiYWNrZ3JvdW5kLXBvc2l0aW9uOiBjZW50ZXI7XHJcbiAgYmFja2dyb3VuZC1zaXplOiAyMDAlO1xyXG4gIHBhZGRpbmc6IDJyZW07XHJcbiAgd2lkdGg6IDM0cmVtO1xyXG5cclxuICAudGl0bGUge1xyXG4gICAgZm9udC1zaXplOiAxLjhyZW07XHJcbiAgICB0ZXh0LWFsaWduOiBjZW50ZXI7XHJcbiAgfVxyXG5cclxuICAub3Blbi1mb3JtIHtcclxuXHJcbiAgICAud2FsbGV0LXBhdGgge1xyXG4gICAgICBmb250LXNpemU6IDEuM3JlbTtcclxuICAgICAgbWFyZ2luOiA1cmVtIDAgMnJlbTtcclxuICAgIH1cclxuXHJcbiAgICAud3JhcC1idXR0b24ge1xyXG4gICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgICAgIG1hcmdpbjogMnJlbSAtMnJlbSAwO1xyXG5cclxuICAgICAgYnV0dG9uIHtcclxuICAgICAgICBmbGV4OiAxIDAgMDtcclxuICAgICAgICBtYXJnaW46IDAgMnJlbSA7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcbn1cclxuIl19 */"

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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span [routerLink]=\"['/main']\">{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\r\n      <span>{{ 'BREADCRUMBS.OPEN_WALLET' | translate }}</span>\r\n    </div>\r\n    <button type=\"button\" class=\"back-btn\" [routerLink]=\"['/main']\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <form class=\"form-open\" [formGroup]=\"openForm\">\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-name\">{{ 'OPEN_WALLET.NAME' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [maxLength]=\"variablesService.maxWalletNameLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"openForm.controls['name'].invalid && (openForm.controls['name'].dirty || openForm.controls['name'].touched)\">\r\n        <div *ngIf=\"openForm.controls['name'].errors['required']\">\r\n          {{ 'OPEN_WALLET.FORM_ERRORS.NAME_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"openForm.controls['name'].errors['duplicate']\">\r\n          {{ 'OPEN_WALLET.FORM_ERRORS.NAME_DUPLICATE' | translate }}\r\n        </div>\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"openForm.get('name').value.length >= variablesService.maxWalletNameLength\">\r\n        {{ 'OPEN_WALLET.FORM_ERRORS.MAX_LENGTH' | translate }}\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-password\">{{ 'OPEN_WALLET.PASS' | translate }}</label>\r\n      <input type=\"password\" id=\"wallet-password\" formControlName=\"password\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\r\n    </div>\r\n\r\n    <div class=\"wrap-buttons\">\r\n      <button type=\"button\" class=\"blue-button create-button\" (click)=\"openWallet()\" [disabled]=\"!openForm.valid\">{{ 'OPEN_WALLET.BUTTON' | translate }}</button>\r\n    </div>\r\n\r\n  </form>\r\n\r\n</div>\r\n\r\n"

/***/ }),

/***/ "./src/app/open-wallet/open-wallet.component.scss":
/*!********************************************************!*\
  !*** ./src/app/open-wallet/open-wallet.component.scss ***!
  \********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-open {\n  margin: 2.4rem 0;\n  width: 50%; }\n  .form-open .wrap-buttons {\n    display: flex;\n    margin: 2.5rem -0.7rem; }\n  .form-open .wrap-buttons button {\n      margin: 0 0.7rem; }\n  .form-open .wrap-buttons button.create-button {\n        flex: 1 1 50%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9vcGVuLXdhbGxldC9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcb3Blbi13YWxsZXRcXG9wZW4td2FsbGV0LmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZ0JBQWdCO0VBQ2hCLFVBQVUsRUFBQTtFQUZaO0lBS0ksYUFBYTtJQUNiLHNCQUFzQixFQUFBO0VBTjFCO01BU00sZ0JBQWdCLEVBQUE7RUFUdEI7UUFZUSxhQUFhLEVBQUEiLCJmaWxlIjoiYXBwL29wZW4td2FsbGV0L29wZW4td2FsbGV0LmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tb3BlbiB7XHJcbiAgbWFyZ2luOiAyLjRyZW0gMDtcclxuICB3aWR0aDogNTAlO1xyXG5cclxuICAud3JhcC1idXR0b25zIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBtYXJnaW46IDIuNXJlbSAtMC43cmVtO1xyXG5cclxuICAgIGJ1dHRvbiB7XHJcbiAgICAgIG1hcmdpbjogMCAwLjdyZW07XHJcblxyXG4gICAgICAmLmNyZWF0ZS1idXR0b24ge1xyXG4gICAgICAgIGZsZXg6IDEgMSA1MCU7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcbn1cclxuIl19 */"

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

module.exports = "<div class=\"head\">\r\n  <div class=\"breadcrumbs\">\r\n    <span [routerLink]=\"'/wallet/' + currentWalletId + '/contracts'\">{{ 'BREADCRUMBS.CONTRACTS' | translate }}</span>\r\n    <span *ngIf=\"newPurchase\">{{ 'BREADCRUMBS.NEW_PURCHASE' | translate }}</span>\r\n    <span *ngIf=\"!newPurchase\">{{ 'BREADCRUMBS.OLD_PURCHASE' | translate }}</span>\r\n  </div>\r\n  <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n    <i class=\"icon back\"></i>\r\n    <span>{{ 'COMMON.BACK' | translate }}</span>\r\n  </button>\r\n</div>\r\n\r\n<form class=\"form-purchase scrolled-content\" [formGroup]=\"purchaseForm\">\r\n\r\n  <div class=\"input-block\">\r\n    <label for=\"purchase-description\">{{ 'PURCHASE.DESCRIPTION' | translate }}</label>\r\n    <input type=\"text\" id=\"purchase-description\" formControlName=\"description\" maxlength=\"100\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    <div class=\"error-block\" *ngIf=\"purchaseForm.controls['description'].invalid && (purchaseForm.controls['description'].dirty || purchaseForm.controls['description'].touched)\">\r\n      <div *ngIf=\"purchaseForm.controls['description'].errors['required']\">\r\n        {{ 'PURCHASE.FORM_ERRORS.DESC_REQUIRED' | translate }}\r\n      </div>\r\n    </div>\r\n    <div class=\"error-block\" *ngIf=\"newPurchase && purchaseForm.controls['description'].value.length >= 100\">\r\n      <div>\r\n        {{ 'PURCHASE.FORM_ERRORS.COMMENT_MAXIMUM' | translate }}\r\n      </div>\r\n    </div>\r\n  </div>\r\n\r\n  <div class=\"input-blocks-row\">\r\n    <div class=\"input-block input-block-alias\">\r\n      <label for=\"purchase-seller\">{{ 'PURCHASE.SELLER' | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-seller\" formControlName=\"seller\" [readonly]=\"!newPurchase\" (mousedown)=\"addressMouseDown($event)\" (contextmenu)=\"(!newPurchase) ? variablesService.onContextMenuOnlyCopy($event, purchaseForm.controls['seller'].value) : variablesService.onContextMenu($event)\">\r\n      <div class=\"alias-dropdown scrolled-content\" *ngIf=\"isOpen\">\r\n        <div *ngFor=\"let item of localAliases\" (click)=\"setAlias(item.name)\">{{item.name}}</div>\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"purchaseForm.controls['seller'].invalid && (purchaseForm.controls['seller'].dirty || purchaseForm.controls['seller'].touched)\">\r\n        <div *ngIf=\"purchaseForm.controls['seller'].errors['required']\">\r\n          {{ 'PURCHASE.FORM_ERRORS.SELLER_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"purchaseForm.controls['seller'].errors['address_not_valid']\">\r\n          {{ 'PURCHASE.FORM_ERRORS.SELLER_NOT_VALID' | translate }}\r\n        </div>\r\n        <div *ngIf=\"purchaseForm.controls['seller'].errors['address_same']\">\r\n          {{ 'PURCHASE.FORM_ERRORS.SELLER_SAME' | translate }}\r\n        </div>\r\n        <div *ngIf=\"purchaseForm.controls['seller'].errors['alias_not_valid']\">\r\n          {{ 'PURCHASE.FORM_ERRORS.ALIAS_NOT_VALID' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"purchase-amount\">{{ 'PURCHASE.AMOUNT' | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-amount\" formControlName=\"amount\" appInputValidate=\"money\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"purchaseForm.controls['amount'].invalid && (purchaseForm.controls['amount'].dirty || purchaseForm.controls['amount'].touched)\">\r\n        <div *ngIf=\"purchaseForm.controls['amount'].errors['required']\">\r\n          {{ 'PURCHASE.FORM_ERRORS.AMOUNT_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"purchaseForm.controls['amount'].errors['amount_zero']\">\r\n          {{ 'PURCHASE.FORM_ERRORS.AMOUNT_ZERO' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n  </div>\r\n\r\n  <div class=\"input-blocks-row\">\r\n    <div class=\"input-block\">\r\n      <label for=\"purchase-your-deposit\">{{ ( (currentContract && !currentContract.is_a) ? 'PURCHASE.BUYER_DEPOSIT' : 'PURCHASE.YOUR_DEPOSIT') | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-your-deposit\" formControlName=\"yourDeposit\" appInputValidate=\"money\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"purchaseForm.controls['yourDeposit'].invalid && (purchaseForm.controls['yourDeposit'].dirty || purchaseForm.controls['yourDeposit'].touched)\">\r\n        <div *ngIf=\"purchaseForm.controls['yourDeposit'].errors['required']\">\r\n          {{ 'PURCHASE.FORM_ERRORS.YOUR_DEPOSIT_REQUIRED' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <div class=\"wrap-label\">\r\n        <label for=\"purchase-seller-deposit\">{{ ( (currentContract && !currentContract.is_a) ? 'PURCHASE.YOUR_DEPOSIT' : 'PURCHASE.SELLER_DEPOSIT') | translate }}</label>\r\n        <div class=\"checkbox-block\">\r\n          <input type=\"checkbox\" id=\"purchase-same-amount\" class=\"style-checkbox\" formControlName=\"sameAmount\" (change)=\"sameAmountChange()\">\r\n          <label for=\"purchase-same-amount\">{{ 'PURCHASE.SAME_AMOUNT' | translate }}</label>\r\n        </div>\r\n      </div>\r\n      <input type=\"text\" readonly *ngIf=\"purchaseForm.controls['sameAmount'].value\" [value]=\"purchaseForm.controls['amount'].value\">\r\n      <input type=\"text\" id=\"purchase-seller-deposit\" *ngIf=\"!purchaseForm.controls['sameAmount'].value\" formControlName=\"sellerDeposit\" appInputValidate=\"money\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"purchaseForm.controls['sellerDeposit'].invalid && (purchaseForm.controls['sellerDeposit'].dirty || purchaseForm.controls['sellerDeposit'].touched)\">\r\n        <div *ngIf=\"purchaseForm.controls['sellerDeposit'].errors['required']\">\r\n          {{ 'PURCHASE.FORM_ERRORS.SELLER_DEPOSIT_REQUIRED' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n  </div>\r\n\r\n  <div class=\"input-block\">\r\n    <label for=\"purchase-comment\">{{ 'PURCHASE.COMMENT' | translate }}</label>\r\n    <input type=\"text\" id=\"purchase-comment\" formControlName=\"comment\" maxlength=\"100\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    <div class=\"error-block\" *ngIf=\"newPurchase && purchaseForm.controls['comment'].value.length >= 100\">\r\n      <div>\r\n        {{ 'PURCHASE.FORM_ERRORS.COMMENT_MAXIMUM' | translate }}\r\n      </div>\r\n    </div>\r\n  </div>\r\n\r\n  <button type=\"button\" class=\"purchase-select\" (click)=\"toggleOptions()\">\r\n    <span>{{ 'PURCHASE.DETAILS' | translate }}</span><i class=\"icon arrow\" [class.down]=\"!additionalOptions\" [class.up]=\"additionalOptions\"></i>\r\n  </button>\r\n\r\n  <div class=\"additional-details\" *ngIf=\"additionalOptions\">\r\n    <div class=\"input-block\">\r\n      <label for=\"purchase-fee\">{{ 'PURCHASE.FEE' | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-fee\" formControlName=\"fee\" readonly>\r\n    </div>\r\n    <div class=\"input-block\" *ngIf=\"newPurchase\">\r\n      <label for=\"purchase-time\">{{ 'PURCHASE.WAITING_TIME' | translate }}</label>\r\n      <ng-select id=\"purchase-time\" class=\"custom-select\"\r\n                 [clearable]=\"false\"\r\n                 [searchable]=\"false\"\r\n                 formControlName=\"time\">\r\n        <ng-option [value]=\"1\">1 {{ 'PURCHASE.HOUR' | translate }}</ng-option>\r\n        <ng-option *ngFor=\"let title of [2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]\" [value]=\"title\">\r\n          {{title}} {{ 'PURCHASE.HOURS' | translate }}\r\n        </ng-option>\r\n      </ng-select>\r\n    </div>\r\n    <div class=\"input-block\">\r\n      <label for=\"purchase-payment\">{{ 'PURCHASE.PAYMENT' | translate }}</label>\r\n      <input type=\"text\" id=\"purchase-payment\" formControlName=\"payment\" [readonly]=\"!newPurchase\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n    </div>\r\n  </div>\r\n\r\n  <button type=\"button\" class=\"blue-button send-button\" *ngIf=\"newPurchase\" [disabled]=\"!purchaseForm.valid\" (click)=\"createPurchase()\">{{ 'PURCHASE.SEND_BUTTON' | translate }}</button>\r\n\r\n  <div class=\"purchase-states\" *ngIf=\"!newPurchase\">\r\n    <ng-container *ngIf=\"currentContract.state == 1 && !currentContract.is_a && currentContract.private_detailes.b_pledge.plus(variablesService.default_fee_big).plus(variablesService.default_fee_big).isGreaterThan(variablesService.currentWallet.unlocked_balance)\">\r\n      <span>{{ 'PURCHASE.NEED_MONEY' | translate }}</span>\r\n    </ng-container>\r\n  </div>\r\n\r\n  <div class=\"purchase-buttons\" *ngIf=\"!newPurchase\">\r\n\r\n    <ng-container *ngIf=\"!currentContract.is_a && currentContract.state == 1\">\r\n      <button type=\"button\" class=\"green-button\" (click)=\"acceptState();\" [disabled]=\"currentContract.private_detailes.b_pledge.plus(variablesService.default_fee_big).plus(variablesService.default_fee_big).isGreaterThan(variablesService.currentWallet.unlocked_balance)\">\r\n        {{'PURCHASE.BUTTON_MAKE_PLEDGE' | translate}}\r\n      </button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"ignoredContract();\">{{'PURCHASE.BUTTON_IGNORE' | translate}}</button>\r\n    </ng-container>\r\n\r\n    <ng-container *ngIf=\"!showNullify && !showTimeSelect && currentContract.is_a && (currentContract.state == 201 || currentContract.state == 2 || currentContract.state == 120 || currentContract.state == 130)\">\r\n      <button type=\"button\" class=\"green-button\" (click)=\"dealsDetailsFinish();\" [disabled]=\"currentContract.cancel_expiration_time == 0 && (currentContract.height == 0 || (variablesService.height_app - currentContract.height) < 10)\">\r\n        {{'PURCHASE.BUTTON_RECEIVED' | translate}}\r\n      </button>\r\n      <button type=\"button\" class=\"turquoise-button\" (click)=\"showNullify = true;\" [disabled]=\"currentContract.cancel_expiration_time == 0 && (currentContract.height == 0 || (variablesService.height_app - currentContract.height) < 10)\">\r\n        {{'PURCHASE.BUTTON_NULLIFY' | translate}}\r\n      </button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"showTimeSelect = true;\" [disabled]=\"currentContract.cancel_expiration_time == 0 && (currentContract.height == 0 || (variablesService.height_app - currentContract.height) < 10)\">\r\n        {{'PURCHASE.BUTTON_CANCEL_BUYER' | translate}}\r\n      </button>\r\n    </ng-container>\r\n\r\n    <ng-container *ngIf=\"!currentContract.is_a && currentContract.state == 5\">\r\n      <button type=\"button\" class=\"turquoise-button\" (click)=\"dealsDetailsDontCanceling();\">{{'PURCHASE.BUTTON_NOT_CANCEL' | translate}}</button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"dealsDetailsSellerCancel();\">{{'PURCHASE.BUTTON_CANCEL_SELLER' | translate}}</button>\r\n    </ng-container>\r\n\r\n  </div>\r\n\r\n  <div class=\"nullify-block-row\" *ngIf=\"showNullify\">\r\n    <div>{{'PURCHASE.NULLIFY_QUESTION' | translate}}</div>\r\n    <div class=\"nullify-block-buttons\">\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"showNullify = false;\">{{ 'PURCHASE.CANCEL' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"productNotGot();\">{{ 'PURCHASE.BUTTON_NULLIFY_SHORT' | translate }}</button>\r\n    </div>\r\n  </div>\r\n\r\n  <div class=\"time-cancel-block-row\" *ngIf=\"showTimeSelect && !newPurchase && currentContract.is_a && (currentContract.state == 201 || currentContract.state == 2 || currentContract.state == 120 || currentContract.state == 130)\">\r\n    <div class=\"time-cancel-block-question\">{{ 'PURCHASE.WAITING_TIME_QUESTION' | translate }}</div>\r\n    <label for=\"purchase-timeCancel\">{{ 'PURCHASE.WAITING_TIME' | translate }}</label>\r\n    <div class=\"input-block\">\r\n      <ng-select id=\"purchase-timeCancel\" class=\"custom-select\"\r\n                 [clearable]=\"false\"\r\n                 [searchable]=\"false\"\r\n                 formControlName=\"timeCancel\">\r\n        <ng-option [value]=\"1\">1 {{ 'PURCHASE.HOUR' | translate }}</ng-option>\r\n        <ng-option *ngFor=\"let title of [2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]\" [value]=\"title\">\r\n          {{title}} {{ 'PURCHASE.HOURS' | translate }}\r\n        </ng-option>\r\n      </ng-select>\r\n    </div>\r\n    <div class=\"time-cancel-block-buttons\">\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"showTimeSelect = false;\">{{ 'PURCHASE.CANCEL' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"dealsDetailsCancel();\">{{ 'PURCHASE.BUTTON_CANCEL_BUYER' | translate }}</button>\r\n    </div>\r\n  </div>\r\n\r\n</form>\r\n\r\n<div class=\"progress-bar-container\">\r\n  <div class=\"progress-bar\">\r\n    <div class=\"progress-bar-full\" [style.width]=\"getProgressBarWidth()\"></div>\r\n  </div>\r\n  <div class=\"progress-labels\">\r\n\r\n    <ng-container *ngIf=\"newPurchase\">\r\n      <span>{{ 'PURCHASE.STATUS_MESSAGES.NEW_PURCHASE' | translate }}</span>\r\n    </ng-container>\r\n\r\n    <ng-container *ngIf=\"!newPurchase && currentContract.is_a\">\r\n      <span *ngIf=\"currentContract.state == 1\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_SELLER' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 110\">{{ 'PURCHASE.STATUS_MESSAGES.IGNORED_SELLER' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 120\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_DELIVERY' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 130\">{{ 'PURCHASE.STATUS_MESSAGES.IGNORED_CANCEL_SELLER' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 140\">{{ 'PURCHASE.STATUS_MESSAGES.EXPIRED' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 2\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_SELLER' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 201\">\r\n        {{ 'PURCHASE.STATUS_MESSAGES.WAITING_CONFIRMATION' | translate }}\r\n        <ng-container *ngIf=\"currentContract.height === 0\">(0/10)</ng-container>\r\n        <ng-container *ngIf=\"currentContract.height !== 0 && (variablesService.height_app - currentContract.height) < 10\">({{variablesService.height_app - currentContract.height}}/10)</ng-container>\r\n      </span>\r\n\r\n      <span *ngIf=\"currentContract.state == 3\">{{ 'PURCHASE.STATUS_MESSAGES.COMPLETED' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 4\" class=\"error-text\">\r\n        {{ 'PURCHASE.STATUS_MESSAGES.NOT_RECEIVED' | translate }}. {{ 'PURCHASE.STATUS_MESSAGES.NULLIFIED' | translate }}\r\n      </span>\r\n\r\n      <span *ngIf=\"currentContract.state == 5\">{{ 'PURCHASE.STATUS_MESSAGES.PROPOSAL_CANCEL_SELLER' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 6\">{{ 'PURCHASE.STATUS_MESSAGES.CANCELLED' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 601\">\r\n        {{ 'PURCHASE.STATUS_MESSAGES.BEING_CANCELLED' | translate }}\r\n        <ng-container *ngIf=\"currentContract.height === 0\">(0/10)</ng-container>\r\n        <ng-container *ngIf=\"currentContract.height !== 0 && (variablesService.height_app - currentContract.height) < 10\">({{variablesService.height_app - currentContract.height}}/10)</ng-container>\r\n      </span>\r\n    </ng-container>\r\n\r\n    <ng-container *ngIf=\"!newPurchase && !currentContract.is_a\">\r\n      <span *ngIf=\"currentContract.state == 1\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_BUYER' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 110\">{{ 'PURCHASE.STATUS_MESSAGES.IGNORED_BUYER' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 130\">{{ 'PURCHASE.STATUS_MESSAGES.IGNORED_CANCEL_BUYER' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 140\">{{ 'PURCHASE.STATUS_MESSAGES.EXPIRED' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 2\">{{ 'PURCHASE.STATUS_MESSAGES.WAITING_DELIVERY' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 201\">\r\n        {{ 'PURCHASE.STATUS_MESSAGES.WAITING_CONFIRMATION' | translate }}\r\n        <ng-container *ngIf=\"currentContract.height === 0\">(0/10)</ng-container>\r\n        <ng-container *ngIf=\"currentContract.height !== 0 && (variablesService.height_app - currentContract.height) < 10\">({{variablesService.height_app - currentContract.height}}/10)</ng-container>\r\n      </span>\r\n\r\n      <span *ngIf=\"currentContract.state == 3\">{{ 'PURCHASE.STATUS_MESSAGES.COMPLETED' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 4\" class=\"error-text\">\r\n        {{ 'PURCHASE.STATUS_MESSAGES.NOT_RECEIVED' | translate }}. {{ 'PURCHASE.STATUS_MESSAGES.NULLIFIED' | translate }}\r\n      </span>\r\n\r\n      <span *ngIf=\"currentContract.state == 5\">{{ 'PURCHASE.STATUS_MESSAGES.PROPOSAL_CANCEL_BUYER' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 6\">{{ 'PURCHASE.STATUS_MESSAGES.CANCELLED' | translate }}</span>\r\n\r\n      <span *ngIf=\"currentContract.state == 601\">\r\n        {{ 'PURCHASE.STATUS_MESSAGES.BEING_CANCELLED' | translate }}\r\n        <ng-container *ngIf=\"currentContract.height === 0\">(0/10)</ng-container>\r\n        <ng-container *ngIf=\"currentContract.height !== 0 && (variablesService.height_app - currentContract.height) < 10\">({{variablesService.height_app - currentContract.height}}/10)</ng-container>\r\n      </span>\r\n    </ng-container>\r\n\r\n  </div>\r\n  <div class=\"progress-time\" *ngIf=\"!newPurchase\">\r\n    <span *ngIf=\"currentContract.is_a && currentContract.state == 1\">{{currentContract.expiration_time | contractTimeLeft: 0}}</span>\r\n    <span *ngIf=\"currentContract.is_a && currentContract.state == 5\">{{currentContract.cancel_expiration_time | contractTimeLeft: 2}}</span>\r\n    <span *ngIf=\"!currentContract.is_a && currentContract.state == 1\">{{currentContract.expiration_time | contractTimeLeft: 1}}</span>\r\n    <span *ngIf=\"!currentContract.is_a && currentContract.state == 5\">{{currentContract.cancel_expiration_time | contractTimeLeft: 1}}</span>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/purchase/purchase.component.scss":
/*!**************************************************!*\
  !*** ./src/app/purchase/purchase.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: flex;\n  flex-direction: column;\n  width: 100%; }\n\n.head {\n  flex: 0 0 auto;\n  box-sizing: content-box;\n  margin: -3rem -3rem 0; }\n\n.form-purchase {\n  flex: 1 1 auto;\n  margin: 1.5rem -3rem 0;\n  padding: 0 3rem;\n  overflow-y: overlay; }\n\n.form-purchase .input-blocks-row {\n    display: flex; }\n\n.form-purchase .input-blocks-row .input-block {\n      flex-basis: 50%; }\n\n.form-purchase .input-blocks-row .input-block:first-child {\n        margin-right: 1.5rem; }\n\n.form-purchase .input-blocks-row .input-block:last-child {\n        margin-left: 1.5rem; }\n\n.form-purchase .input-blocks-row .input-block .checkbox-block {\n        display: flex; }\n\n.form-purchase .purchase-select {\n    display: flex;\n    align-items: center;\n    background: transparent;\n    border: none;\n    font-size: 1.3rem;\n    line-height: 1.3rem;\n    margin: 1.5rem 0 0;\n    padding: 0;\n    width: 100%;\n    max-width: 15rem;\n    height: 1.3rem; }\n\n.form-purchase .purchase-select .arrow {\n      margin-left: 1rem;\n      width: 0.8rem;\n      height: 0.8rem; }\n\n.form-purchase .purchase-select .arrow.down {\n        -webkit-mask: url('arrow-down.svg') no-repeat center;\n                mask: url('arrow-down.svg') no-repeat center; }\n\n.form-purchase .purchase-select .arrow.up {\n        -webkit-mask: url('arrow-up.svg') no-repeat center;\n                mask: url('arrow-up.svg') no-repeat center; }\n\n.form-purchase .additional-details {\n    display: flex;\n    margin-top: 1.5rem;\n    padding: 0.5rem 0 2rem; }\n\n.form-purchase .additional-details > div {\n      flex-basis: 25%; }\n\n.form-purchase .additional-details > div:first-child {\n        padding-left: 1.5rem;\n        padding-right: 1rem; }\n\n.form-purchase .additional-details > div:last-child {\n        padding-left: 1rem;\n        padding-right: 1.5rem; }\n\n.form-purchase .purchase-states {\n    display: flex;\n    flex-direction: column;\n    align-items: center;\n    justify-content: center;\n    font-size: 1.2rem;\n    line-height: 2.9rem; }\n\n.form-purchase .send-button {\n    margin: 2.4rem 0;\n    width: 100%;\n    max-width: 15rem; }\n\n.form-purchase .purchase-buttons {\n    display: flex;\n    justify-content: flex-start;\n    margin: 2.4rem -0.5rem; }\n\n.form-purchase .purchase-buttons button {\n      flex: 0 1 33%;\n      margin: 0 0.5rem; }\n\n.form-purchase .nullify-block-row {\n    display: flex;\n    flex-direction: column;\n    align-items: center;\n    justify-content: center; }\n\n.form-purchase .nullify-block-row .nullify-block-buttons {\n      display: flex;\n      align-items: center;\n      justify-content: center;\n      margin: 1rem 0;\n      width: 100%; }\n\n.form-purchase .nullify-block-row .nullify-block-buttons button {\n        flex: 0 1 25%;\n        margin: 0 0.5rem; }\n\n.form-purchase .time-cancel-block-row {\n    display: flex;\n    flex-direction: column;\n    align-items: center;\n    justify-content: center; }\n\n.form-purchase .time-cancel-block-row .time-cancel-block-question {\n      margin-bottom: 1rem; }\n\n.form-purchase .time-cancel-block-row .input-block {\n      width: 25%; }\n\n.form-purchase .time-cancel-block-row label {\n      margin-bottom: 1rem; }\n\n.form-purchase .time-cancel-block-row .time-cancel-block-buttons {\n      display: flex;\n      align-items: center;\n      justify-content: center;\n      margin: 1rem 0;\n      width: 100%; }\n\n.form-purchase .time-cancel-block-row .time-cancel-block-buttons button {\n        flex: 0 1 25%;\n        margin: 0 0.5rem; }\n\n.progress-bar-container {\n  position: absolute;\n  bottom: 0;\n  left: 0;\n  padding: 0 3rem;\n  width: 100%;\n  height: 3rem; }\n\n.progress-bar-container .progress-bar {\n    position: absolute;\n    top: -0.7rem;\n    left: 0;\n    margin: 0 3rem;\n    width: calc(100% - 6rem);\n    height: 0.7rem; }\n\n.progress-bar-container .progress-bar .progress-bar-full {\n      height: 0.7rem; }\n\n.progress-bar-container .progress-labels {\n    display: flex;\n    align-items: center;\n    justify-content: center;\n    font-size: 1.2rem;\n    height: 100%; }\n\n.progress-bar-container .progress-time {\n    position: absolute;\n    top: -3rem;\n    left: 50%;\n    transform: translateX(-50%);\n    font-size: 1.2rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9wdXJjaGFzZS9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxccHVyY2hhc2VcXHB1cmNoYXNlLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsYUFBYTtFQUNiLHNCQUFzQjtFQUN0QixXQUFXLEVBQUE7O0FBR2I7RUFDRSxjQUFjO0VBQ2QsdUJBQXVCO0VBQ3ZCLHFCQUFxQixFQUFBOztBQUd2QjtFQUNFLGNBQWM7RUFDZCxzQkFBc0I7RUFDdEIsZUFBZTtFQUNmLG1CQUFtQixFQUFBOztBQUpyQjtJQU9JLGFBQWEsRUFBQTs7QUFQakI7TUFVTSxlQUFlLEVBQUE7O0FBVnJCO1FBYVEsb0JBQW9CLEVBQUE7O0FBYjVCO1FBaUJRLG1CQUFtQixFQUFBOztBQWpCM0I7UUFxQlEsYUFBYSxFQUFBOztBQXJCckI7SUEyQkksYUFBYTtJQUNiLG1CQUFtQjtJQUNuQix1QkFBdUI7SUFDdkIsWUFBWTtJQUNaLGlCQUFpQjtJQUNqQixtQkFBbUI7SUFDbkIsa0JBQWtCO0lBQ2xCLFVBQVU7SUFDVixXQUFXO0lBQ1gsZ0JBQWdCO0lBQ2hCLGNBQWMsRUFBQTs7QUFyQ2xCO01Bd0NNLGlCQUFpQjtNQUNqQixhQUFhO01BQ2IsY0FBYyxFQUFBOztBQTFDcEI7UUE2Q1Esb0RBQTREO2dCQUE1RCw0Q0FBNEQsRUFBQTs7QUE3Q3BFO1FBaURRLGtEQUEwRDtnQkFBMUQsMENBQTBELEVBQUE7O0FBakRsRTtJQXVESSxhQUFhO0lBQ2Isa0JBQWtCO0lBQ2xCLHNCQUFzQixFQUFBOztBQXpEMUI7TUE0RE0sZUFBZSxFQUFBOztBQTVEckI7UUErRFEsb0JBQW9CO1FBQ3BCLG1CQUFtQixFQUFBOztBQWhFM0I7UUFvRVEsa0JBQWtCO1FBQ2xCLHFCQUFxQixFQUFBOztBQXJFN0I7SUEyRUksYUFBYTtJQUNiLHNCQUFzQjtJQUN0QixtQkFBbUI7SUFDbkIsdUJBQXVCO0lBQ3ZCLGlCQUFpQjtJQUNqQixtQkFBbUIsRUFBQTs7QUFoRnZCO0lBb0ZJLGdCQUFnQjtJQUNoQixXQUFXO0lBQ1gsZ0JBQWdCLEVBQUE7O0FBdEZwQjtJQTBGSSxhQUFhO0lBQ2IsMkJBQTJCO0lBQzNCLHNCQUFzQixFQUFBOztBQTVGMUI7TUErRk0sYUFBYTtNQUNiLGdCQUFnQixFQUFBOztBQWhHdEI7SUFxR0ksYUFBYTtJQUNiLHNCQUFzQjtJQUN0QixtQkFBbUI7SUFDbkIsdUJBQXVCLEVBQUE7O0FBeEczQjtNQTJHTSxhQUFhO01BQ2IsbUJBQW1CO01BQ25CLHVCQUF1QjtNQUN2QixjQUFjO01BQ2QsV0FBVyxFQUFBOztBQS9HakI7UUFrSFEsYUFBYTtRQUNiLGdCQUFnQixFQUFBOztBQW5IeEI7SUF5SEksYUFBYTtJQUNiLHNCQUFzQjtJQUN0QixtQkFBbUI7SUFDbkIsdUJBQXVCLEVBQUE7O0FBNUgzQjtNQStITSxtQkFBbUIsRUFBQTs7QUEvSHpCO01BbUlNLFVBQVUsRUFBQTs7QUFuSWhCO01BdUlNLG1CQUFtQixFQUFBOztBQXZJekI7TUEySU0sYUFBYTtNQUNiLG1CQUFtQjtNQUNuQix1QkFBdUI7TUFDdkIsY0FBYztNQUNkLFdBQVcsRUFBQTs7QUEvSWpCO1FBa0pRLGFBQWE7UUFDYixnQkFBZ0IsRUFBQTs7QUFPeEI7RUFDRSxrQkFBa0I7RUFDbEIsU0FBUztFQUNULE9BQU87RUFDUCxlQUFlO0VBQ2YsV0FBVztFQUNYLFlBQVksRUFBQTs7QUFOZDtJQVNJLGtCQUFrQjtJQUNsQixZQUFZO0lBQ1osT0FBTztJQUNQLGNBQWM7SUFDZCx3QkFBd0I7SUFDeEIsY0FBYyxFQUFBOztBQWRsQjtNQWlCTSxjQUFjLEVBQUE7O0FBakJwQjtJQXNCSSxhQUFhO0lBQ2IsbUJBQW1CO0lBQ25CLHVCQUF1QjtJQUN2QixpQkFBaUI7SUFDakIsWUFBWSxFQUFBOztBQTFCaEI7SUE4Qkksa0JBQWtCO0lBQ2xCLFVBQVU7SUFDVixTQUFTO0lBQ1QsMkJBQTJCO0lBQzNCLGlCQUFpQixFQUFBIiwiZmlsZSI6ImFwcC9wdXJjaGFzZS9wdXJjaGFzZS5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgd2lkdGg6IDEwMCU7XHJcbn1cclxuXHJcbi5oZWFkIHtcclxuICBmbGV4OiAwIDAgYXV0bztcclxuICBib3gtc2l6aW5nOiBjb250ZW50LWJveDtcclxuICBtYXJnaW46IC0zcmVtIC0zcmVtIDA7XHJcbn1cclxuXHJcbi5mb3JtLXB1cmNoYXNlIHtcclxuICBmbGV4OiAxIDEgYXV0bztcclxuICBtYXJnaW46IDEuNXJlbSAtM3JlbSAwO1xyXG4gIHBhZGRpbmc6IDAgM3JlbTtcclxuICBvdmVyZmxvdy15OiBvdmVybGF5O1xyXG5cclxuICAuaW5wdXQtYmxvY2tzLXJvdyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG5cclxuICAgIC5pbnB1dC1ibG9jayB7XHJcbiAgICAgIGZsZXgtYmFzaXM6IDUwJTtcclxuXHJcbiAgICAgICY6Zmlyc3QtY2hpbGQge1xyXG4gICAgICAgIG1hcmdpbi1yaWdodDogMS41cmVtO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmOmxhc3QtY2hpbGQge1xyXG4gICAgICAgIG1hcmdpbi1sZWZ0OiAxLjVyZW07XHJcbiAgICAgIH1cclxuXHJcbiAgICAgIC5jaGVja2JveC1ibG9jayB7XHJcbiAgICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnB1cmNoYXNlLXNlbGVjdCB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGJhY2tncm91bmQ6IHRyYW5zcGFyZW50O1xyXG4gICAgYm9yZGVyOiBub25lO1xyXG4gICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICBsaW5lLWhlaWdodDogMS4zcmVtO1xyXG4gICAgbWFyZ2luOiAxLjVyZW0gMCAwO1xyXG4gICAgcGFkZGluZzogMDtcclxuICAgIHdpZHRoOiAxMDAlO1xyXG4gICAgbWF4LXdpZHRoOiAxNXJlbTtcclxuICAgIGhlaWdodDogMS4zcmVtO1xyXG5cclxuICAgIC5hcnJvdyB7XHJcbiAgICAgIG1hcmdpbi1sZWZ0OiAxcmVtO1xyXG4gICAgICB3aWR0aDogMC44cmVtO1xyXG4gICAgICBoZWlnaHQ6IDAuOHJlbTtcclxuXHJcbiAgICAgICYuZG93biB7XHJcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL2Fycm93LWRvd24uc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLnVwIHtcclxuICAgICAgICBtYXNrOiB1cmwofnNyYy9hc3NldHMvaWNvbnMvYXJyb3ctdXAuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAuYWRkaXRpb25hbC1kZXRhaWxzIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBtYXJnaW4tdG9wOiAxLjVyZW07XHJcbiAgICBwYWRkaW5nOiAwLjVyZW0gMCAycmVtO1xyXG5cclxuICAgID4gZGl2IHtcclxuICAgICAgZmxleC1iYXNpczogMjUlO1xyXG5cclxuICAgICAgJjpmaXJzdC1jaGlsZCB7XHJcbiAgICAgICAgcGFkZGluZy1sZWZ0OiAxLjVyZW07XHJcbiAgICAgICAgcGFkZGluZy1yaWdodDogMXJlbTtcclxuICAgICAgfVxyXG5cclxuICAgICAgJjpsYXN0LWNoaWxkIHtcclxuICAgICAgICBwYWRkaW5nLWxlZnQ6IDFyZW07XHJcbiAgICAgICAgcGFkZGluZy1yaWdodDogMS41cmVtO1xyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAucHVyY2hhc2Utc3RhdGVzIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xyXG4gICAgZm9udC1zaXplOiAxLjJyZW07XHJcbiAgICBsaW5lLWhlaWdodDogMi45cmVtO1xyXG4gIH1cclxuXHJcbiAgLnNlbmQtYnV0dG9uIHtcclxuICAgIG1hcmdpbjogMi40cmVtIDA7XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIG1heC13aWR0aDogMTVyZW07XHJcbiAgfVxyXG5cclxuICAucHVyY2hhc2UtYnV0dG9ucyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAganVzdGlmeS1jb250ZW50OiBmbGV4LXN0YXJ0O1xyXG4gICAgbWFyZ2luOiAyLjRyZW0gLTAuNXJlbTtcclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBmbGV4OiAwIDEgMzMlO1xyXG4gICAgICBtYXJnaW46IDAgMC41cmVtO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLm51bGxpZnktYmxvY2stcm93IHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xyXG5cclxuICAgIC5udWxsaWZ5LWJsb2NrLWJ1dHRvbnMge1xyXG4gICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgICAgbWFyZ2luOiAxcmVtIDA7XHJcbiAgICAgIHdpZHRoOiAxMDAlO1xyXG5cclxuICAgICAgYnV0dG9uIHtcclxuICAgICAgICBmbGV4OiAwIDEgMjUlO1xyXG4gICAgICAgIG1hcmdpbjogMCAwLjVyZW07XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcblxyXG4gIC50aW1lLWNhbmNlbC1ibG9jay1yb3cge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XHJcblxyXG4gICAgLnRpbWUtY2FuY2VsLWJsb2NrLXF1ZXN0aW9uIHtcclxuICAgICAgbWFyZ2luLWJvdHRvbTogMXJlbTtcclxuICAgIH1cclxuXHJcbiAgICAuaW5wdXQtYmxvY2sge1xyXG4gICAgICB3aWR0aDogMjUlO1xyXG4gICAgfVxyXG5cclxuICAgIGxhYmVsIHtcclxuICAgICAgbWFyZ2luLWJvdHRvbTogMXJlbTtcclxuICAgIH1cclxuXHJcbiAgICAudGltZS1jYW5jZWwtYmxvY2stYnV0dG9ucyB7XHJcbiAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xyXG4gICAgICBtYXJnaW46IDFyZW0gMDtcclxuICAgICAgd2lkdGg6IDEwMCU7XHJcblxyXG4gICAgICBidXR0b24ge1xyXG4gICAgICAgIGZsZXg6IDAgMSAyNSU7XHJcbiAgICAgICAgbWFyZ2luOiAwIDAuNXJlbTtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbn1cclxuXHJcbi5wcm9ncmVzcy1iYXItY29udGFpbmVyIHtcclxuICBwb3NpdGlvbjogYWJzb2x1dGU7XHJcbiAgYm90dG9tOiAwO1xyXG4gIGxlZnQ6IDA7XHJcbiAgcGFkZGluZzogMCAzcmVtO1xyXG4gIHdpZHRoOiAxMDAlO1xyXG4gIGhlaWdodDogM3JlbTtcclxuXHJcbiAgLnByb2dyZXNzLWJhciB7XHJcbiAgICBwb3NpdGlvbjogYWJzb2x1dGU7XHJcbiAgICB0b3A6IC0wLjdyZW07XHJcbiAgICBsZWZ0OiAwO1xyXG4gICAgbWFyZ2luOiAwIDNyZW07XHJcbiAgICB3aWR0aDogY2FsYygxMDAlIC0gNnJlbSk7XHJcbiAgICBoZWlnaHQ6IDAuN3JlbTtcclxuXHJcbiAgICAucHJvZ3Jlc3MtYmFyLWZ1bGwge1xyXG4gICAgICBoZWlnaHQ6IDAuN3JlbTtcclxuICAgIH1cclxuICB9XHJcblxyXG4gIC5wcm9ncmVzcy1sYWJlbHMge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgIGZvbnQtc2l6ZTogMS4ycmVtO1xyXG4gICAgaGVpZ2h0OiAxMDAlO1xyXG4gIH1cclxuXHJcbiAgLnByb2dyZXNzLXRpbWUge1xyXG4gICAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gICAgdG9wOiAtM3JlbTtcclxuICAgIGxlZnQ6IDUwJTtcclxuICAgIHRyYW5zZm9ybTogdHJhbnNsYXRlWCgtNTAlKTtcclxuICAgIGZvbnQtc2l6ZTogMS4ycmVtO1xyXG4gIH1cclxufVxyXG4iXX0= */"

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

module.exports = "<div class=\"wrap-qr\">\r\n  <img src=\"{{qrImageSrc}}\" alt=\"qr-code\">\r\n  <div class=\"wrap-address\">\r\n    <div class=\"address\">{{variablesService.currentWallet.address}}</div>\r\n    <button type=\"button\" class=\"btn-copy-address\" [class.copy]=\"!copyAnimation\" [class.copied]=\"copyAnimation\" (click)=\"copyAddress()\"></button>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/receive/receive.component.scss":
/*!************************************************!*\
  !*** ./src/app/receive/receive.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.wrap-qr {\n  display: flex;\n  flex-direction: column;\n  align-items: center; }\n\n.wrap-qr img {\n    margin: 4rem 0; }\n\n.wrap-qr .wrap-address {\n    display: flex;\n    align-items: center;\n    font-size: 1.4rem;\n    line-height: 2.7rem; }\n\n.wrap-qr .wrap-address .btn-copy-address {\n      margin-left: 1.2rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.wrap-qr .wrap-address .btn-copy-address.copy {\n        -webkit-mask: url('copy.svg') no-repeat center;\n                mask: url('copy.svg') no-repeat center; }\n\n.wrap-qr .wrap-address .btn-copy-address.copy:hover {\n          opacity: 0.75; }\n\n.wrap-qr .wrap-address .btn-copy-address.copied {\n        -webkit-mask: url('complete-testwallet.svg') no-repeat center;\n                mask: url('complete-testwallet.svg') no-repeat center; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9yZWNlaXZlL0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxyZWNlaXZlXFxyZWNlaXZlLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsV0FBVyxFQUFBOztBQUdiO0VBQ0UsYUFBYTtFQUNiLHNCQUFzQjtFQUN0QixtQkFBbUIsRUFBQTs7QUFIckI7SUFNSSxjQUFjLEVBQUE7O0FBTmxCO0lBVUksYUFBYTtJQUNiLG1CQUFtQjtJQUNuQixpQkFBaUI7SUFDakIsbUJBQW1CLEVBQUE7O0FBYnZCO01BZ0JNLG1CQUFtQjtNQUNuQixhQUFhO01BQ2IsY0FBYyxFQUFBOztBQWxCcEI7UUFxQlEsOENBQXVEO2dCQUF2RCxzQ0FBdUQsRUFBQTs7QUFyQi9EO1VBd0JVLGFBQWEsRUFBQTs7QUF4QnZCO1FBNkJRLDZEQUFzRTtnQkFBdEUscURBQXNFLEVBQUEiLCJmaWxlIjoiYXBwL3JlY2VpdmUvcmVjZWl2ZS5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICB3aWR0aDogMTAwJTtcclxufVxyXG5cclxuLndyYXAtcXIge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG5cclxuICBpbWcge1xyXG4gICAgbWFyZ2luOiA0cmVtIDA7XHJcbiAgfVxyXG5cclxuICAud3JhcC1hZGRyZXNzIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgZm9udC1zaXplOiAxLjRyZW07XHJcbiAgICBsaW5lLWhlaWdodDogMi43cmVtO1xyXG5cclxuICAgIC5idG4tY29weS1hZGRyZXNzIHtcclxuICAgICAgbWFyZ2luLWxlZnQ6IDEuMnJlbTtcclxuICAgICAgd2lkdGg6IDEuN3JlbTtcclxuICAgICAgaGVpZ2h0OiAxLjdyZW07XHJcblxyXG4gICAgICAmLmNvcHkge1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvY29weS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcblxyXG4gICAgICAgICY6aG92ZXIge1xyXG4gICAgICAgICAgb3BhY2l0eTogMC43NTtcclxuICAgICAgICB9XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYuY29waWVkIHtcclxuICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2NvbXBsZXRlLXRlc3R3YWxsZXQuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcbiJdfQ== */"

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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span [routerLink]=\"['/main']\">{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\r\n      <span>{{ 'BREADCRUMBS.RESTORE_WALLET' | translate }}</span>\r\n    </div>\r\n    <button type=\"button\" class=\"back-btn\" [routerLink]=\"['/main']\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <form class=\"form-restore\" [formGroup]=\"restoreForm\">\r\n\r\n    <div class=\"input-block half-block\">\r\n      <label for=\"wallet-name\">{{ 'RESTORE_WALLET.LABEL_NAME' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [attr.readonly]=\"walletSaved ? '' : null\" [maxLength]=\"variablesService.maxWalletNameLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"restoreForm.controls['name'].invalid && (restoreForm.controls['name'].dirty || restoreForm.controls['name'].touched)\">\r\n        <div *ngIf=\"restoreForm.controls['name'].errors['required']\">\r\n          {{ 'RESTORE_WALLET.FORM_ERRORS.NAME_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"restoreForm.controls['name'].errors['duplicate']\">\r\n          {{ 'RESTORE_WALLET.FORM_ERRORS.NAME_DUPLICATE' | translate }}\r\n        </div>\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"restoreForm.get('name').value.length >= variablesService.maxWalletNameLength\">\r\n        {{ 'RESTORE_WALLET.FORM_ERRORS.MAX_LENGTH' | translate }}\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block half-block\">\r\n      <label for=\"wallet-password\">{{ 'RESTORE_WALLET.PASS' | translate }}</label>\r\n      <input type=\"password\" id=\"wallet-password\" formControlName=\"password\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\r\n    </div>\r\n\r\n    <div class=\"input-block half-block\">\r\n      <label for=\"confirm-wallet-password\">{{ 'RESTORE_WALLET.CONFIRM' | translate }}</label>\r\n      <input type=\"password\" id=\"confirm-wallet-password\" formControlName=\"confirm\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\">\r\n      <div class=\"error-block\" *ngIf=\"restoreForm.controls['password'].dirty && restoreForm.controls['confirm'].dirty && restoreForm.errors\">\r\n        <div *ngIf=\"restoreForm.errors['confirm_mismatch']\">\r\n          {{ 'RESTORE_WALLET.FORM_ERRORS.CONFIRM_NOT_MATCH' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"phrase-key\">{{ 'RESTORE_WALLET.LABEL_PHRASE_KEY' | translate }}</label>\r\n      <input type=\"text\" id=\"phrase-key\" formControlName=\"key\" [attr.readonly]=\"walletSaved ? '' : null\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"restoreForm.controls['key'].invalid && (restoreForm.controls['key'].dirty || restoreForm.controls['key'].touched)\">\r\n        <div *ngIf=\"restoreForm.controls['key'].errors['required']\">\r\n          {{ 'RESTORE_WALLET.FORM_ERRORS.KEY_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"restoreForm.controls['key'].errors['key_not_valid']\">\r\n          {{ 'RESTORE_WALLET.FORM_ERRORS.KEY_NOT_VALID' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"wrap-buttons\">\r\n      <button type=\"button\" class=\"transparent-button\" *ngIf=\"walletSaved\" disabled><i class=\"icon\"></i>{{walletSavedName}}</button>\r\n      <button type=\"button\" class=\"blue-button select-button\" (click)=\"saveWallet()\" [disabled]=\"!restoreForm.valid\" *ngIf=\"!walletSaved\">{{ 'RESTORE_WALLET.BUTTON_SELECT' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button create-button\" (click)=\"createWallet()\" [disabled]=\"!walletSaved\">{{ 'RESTORE_WALLET.BUTTON_CREATE' | translate }}</button>\r\n    </div>\r\n\r\n  </form>\r\n\r\n</div>\r\n\r\n<app-progress-container [width]=\"progressWidth\" [labels]=\"['PROGRESS.ADD_WALLET', 'PROGRESS.SELECT_LOCATION', 'PROGRESS.RESTORE_WALLET']\"></app-progress-container>\r\n"

/***/ }),

/***/ "./src/app/restore-wallet/restore-wallet.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/restore-wallet/restore-wallet.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: relative; }\n\n.form-restore {\n  margin: 2.4rem 0;\n  width: 100%; }\n\n.form-restore .input-block.half-block {\n    width: 50%; }\n\n.form-restore .wrap-buttons {\n    display: flex;\n    margin: 2.5rem -0.7rem;\n    width: 50%; }\n\n.form-restore .wrap-buttons button {\n      margin: 0 0.7rem; }\n\n.form-restore .wrap-buttons button.transparent-button {\n        flex-basis: 50%; }\n\n.form-restore .wrap-buttons button.select-button {\n        flex-basis: 60%; }\n\n.form-restore .wrap-buttons button.create-button {\n        flex: 1 1 50%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9yZXN0b3JlLXdhbGxldC9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxccmVzdG9yZS13YWxsZXRcXHJlc3RvcmUtd2FsbGV0LmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCLEVBQUE7O0FBR3BCO0VBQ0UsZ0JBQWdCO0VBQ2hCLFdBQVcsRUFBQTs7QUFGYjtJQU9NLFVBQVUsRUFBQTs7QUFQaEI7SUFZSSxhQUFhO0lBQ2Isc0JBQXNCO0lBQ3RCLFVBQVUsRUFBQTs7QUFkZDtNQWlCTSxnQkFBZ0IsRUFBQTs7QUFqQnRCO1FBb0JRLGVBQWUsRUFBQTs7QUFwQnZCO1FBd0JRLGVBQWUsRUFBQTs7QUF4QnZCO1FBNEJRLGFBQWEsRUFBQSIsImZpbGUiOiJhcHAvcmVzdG9yZS13YWxsZXQvcmVzdG9yZS13YWxsZXQuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XHJcbiAgcG9zaXRpb246IHJlbGF0aXZlO1xyXG59XHJcblxyXG4uZm9ybS1yZXN0b3JlIHtcclxuICBtYXJnaW46IDIuNHJlbSAwO1xyXG4gIHdpZHRoOiAxMDAlO1xyXG5cclxuICAuaW5wdXQtYmxvY2sge1xyXG5cclxuICAgICYuaGFsZi1ibG9jayB7XHJcbiAgICAgIHdpZHRoOiA1MCU7XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAud3JhcC1idXR0b25zIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBtYXJnaW46IDIuNXJlbSAtMC43cmVtO1xyXG4gICAgd2lkdGg6IDUwJTtcclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBtYXJnaW46IDAgMC43cmVtO1xyXG5cclxuICAgICAgJi50cmFuc3BhcmVudC1idXR0b24ge1xyXG4gICAgICAgIGZsZXgtYmFzaXM6IDUwJTtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi5zZWxlY3QtYnV0dG9uIHtcclxuICAgICAgICBmbGV4LWJhc2lzOiA2MCU7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYuY3JlYXRlLWJ1dHRvbiB7XHJcbiAgICAgICAgZmxleDogMSAxIDUwJTtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span [routerLink]=\"['/main']\">{{ 'BREADCRUMBS.ADD_WALLET' | translate }}</span>\r\n      <span>{{ 'BREADCRUMBS.SAVE_PHRASE' | translate }}</span>\r\n    </div>\r\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <h3 class=\"seed-phrase-title\">{{ 'SEED_PHRASE.TITLE' | translate }}</h3>\r\n\r\n  <div class=\"seed-phrase-content\" (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, seedPhrase)\">\r\n    <ng-container *ngFor=\"let word of seedPhrase.split(' '); let index = index\">\r\n      <div class=\"word\">{{(index + 1) + '. ' + word}}</div>\r\n    </ng-container>\r\n  </div>\r\n\r\n  <div class=\"wrap-buttons\">\r\n    <button type=\"button\" class=\"blue-button seed-phrase-button\" (click)=\"runWallet()\">{{ 'SEED_PHRASE.BUTTON_CREATE_ACCOUNT' | translate }}</button>\r\n    <button type=\"button\" class=\"blue-button copy-button\" (click)=\"copySeedPhrase()\">{{ 'SEED_PHRASE.BUTTON_COPY' | translate }}</button>\r\n  </div>\r\n</div>\r\n\r\n<app-progress-container [width]=\"'100%'\" [labels]=\"['PROGRESS.ADD_WALLET', 'PROGRESS.SELECT_LOCATION', 'PROGRESS.CREATE_WALLET']\"></app-progress-container>\r\n"

/***/ }),

/***/ "./src/app/seed-phrase/seed-phrase.component.scss":
/*!********************************************************!*\
  !*** ./src/app/seed-phrase/seed-phrase.component.scss ***!
  \********************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: relative; }\n\n.seed-phrase-title {\n  line-height: 2.2rem;\n  padding: 2.2rem 0; }\n\n.seed-phrase-content {\n  display: flex;\n  flex-direction: column;\n  flex-wrap: wrap;\n  padding: 1.4rem;\n  width: 100%;\n  height: 12rem; }\n\n.seed-phrase-content .word {\n    line-height: 2.2rem;\n    max-width: 13rem; }\n\n.wrap-buttons {\n  display: flex; }\n\n.wrap-buttons .seed-phrase-button {\n    margin: 2.8rem 0;\n    width: 25%;\n    min-width: 1.5rem; }\n\n.wrap-buttons .copy-button {\n    margin: 2.8rem 1rem;\n    width: 25%;\n    min-width: 1.5rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9zZWVkLXBocmFzZS9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcc2VlZC1waHJhc2VcXHNlZWQtcGhyYXNlLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCLEVBQUE7O0FBR3BCO0VBQ0UsbUJBQW1CO0VBQ25CLGlCQUFpQixFQUFBOztBQUduQjtFQUNFLGFBQWE7RUFDYixzQkFBc0I7RUFDdEIsZUFBZTtFQUNmLGVBQWU7RUFDZixXQUFXO0VBQ1gsYUFBYSxFQUFBOztBQU5mO0lBU0ksbUJBQW1CO0lBQ25CLGdCQUFnQixFQUFBOztBQUlwQjtFQUNFLGFBQWEsRUFBQTs7QUFEZjtJQUlJLGdCQUFnQjtJQUNoQixVQUFVO0lBQ1YsaUJBQWlCLEVBQUE7O0FBTnJCO0lBVUksbUJBQW1CO0lBQ25CLFVBQVU7SUFDVixpQkFBaUIsRUFBQSIsImZpbGUiOiJhcHAvc2VlZC1waHJhc2Uvc2VlZC1waHJhc2UuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XHJcbiAgcG9zaXRpb246IHJlbGF0aXZlO1xyXG59XHJcblxyXG4uc2VlZC1waHJhc2UtdGl0bGUge1xyXG4gIGxpbmUtaGVpZ2h0OiAyLjJyZW07XHJcbiAgcGFkZGluZzogMi4ycmVtIDA7XHJcbn1cclxuXHJcbi5zZWVkLXBocmFzZS1jb250ZW50IHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgZmxleC13cmFwOiB3cmFwO1xyXG4gIHBhZGRpbmc6IDEuNHJlbTtcclxuICB3aWR0aDogMTAwJTtcclxuICBoZWlnaHQ6IDEycmVtO1xyXG5cclxuICAud29yZCB7XHJcbiAgICBsaW5lLWhlaWdodDogMi4ycmVtO1xyXG4gICAgbWF4LXdpZHRoOiAxM3JlbTtcclxuICB9XHJcbn1cclxuXHJcbi53cmFwLWJ1dHRvbnMge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcblxyXG4gIC5zZWVkLXBocmFzZS1idXR0b24ge1xyXG4gICAgbWFyZ2luOiAyLjhyZW0gMDtcclxuICAgIHdpZHRoOiAyNSU7XHJcbiAgICBtaW4td2lkdGg6IDEuNXJlbTtcclxuICB9XHJcblxyXG4gIC5jb3B5LWJ1dHRvbiB7XHJcbiAgICBtYXJnaW46IDIuOHJlbSAxcmVtO1xyXG4gICAgd2lkdGg6IDI1JTtcclxuICAgIG1pbi13aWR0aDogMS41cmVtO1xyXG4gIH1cclxufVxyXG5cclxuIl19 */"

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

/***/ "./src/app/send-modal/send-modal.component.html":
/*!******************************************************!*\
  !*** ./src/app/send-modal/send-modal.component.html ***!
  \******************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<div class=\"modal\">\r\n  <div class=\"title\">\r\n    <span>{{ 'CONFIRM.TITLE' | translate }}</span>\r\n  </div>\r\n  <div class=\"content\">\r\n    <div class=\"message-container\">\r\n      <div class=\"message-block\">\r\n        <div class=\"message-label\">{{ 'CONFIRM.MESSAGE.SEND' | translate }}</div>\r\n        <div class=\"message-text\">{{ form.get('amount').value }} {{variablesService.defaultCurrency}}</div>\r\n      </div>\r\n      <div class=\"message-block\">\r\n        <div class=\"message-label\">{{ 'CONFIRM.MESSAGE.FROM' | translate }}</div>\r\n        <div class=\"message-text\">{{ variablesService.currentWallet.address }}</div>\r\n      </div>\r\n      <div class=\"message-block\">\r\n        <div class=\"message-label\">{{ 'CONFIRM.MESSAGE.TO' | translate }}</div>\r\n        <div class=\"message-text\">{{ form.get('address').value }}</div>\r\n      </div>\r\n      <ng-container *ngIf=\"form.get('comment').value != ''\">\r\n        <div class=\"message-block\" *ngIf=\"form.get('comment').value != null\">\r\n          <div class=\"message-label\">{{ 'CONFIRM.MESSAGE.COMMENT' | translate }}</div>\r\n          <div class=\"message-text\">{{ form.get('comment').value }}</div>\r\n        </div>  \r\n      </ng-container>   \r\n    </div>\r\n  </div>\r\n  <div class=\"wrapper-buttons\">\r\n    <button type=\"button\" class=\"blue-button\" (click)=\"confirm()\">{{ 'CONFIRM.BUTTON_CONFIRM' | translate }}</button>\r\n    <button type=\"button\" class=\"blue-button\" (click)=\"onClose()\">{{ 'CONFIRM.BUTTON_CANCEL' | translate }}</button>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/send-modal/send-modal.component.scss":
/*!******************************************************!*\
  !*** ./src/app/send-modal/send-modal.component.scss ***!
  \******************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: fixed;\n  top: 0;\n  bottom: 0;\n  left: 0;\n  right: 0;\n  display: flex;\n  align-items: center;\n  justify-content: center;\n  background: rgba(255, 255, 255, 0.25); }\n\n.modal {\n  position: relative;\n  display: flex;\n  flex-direction: column;\n  background-position: center;\n  background-size: 200%;\n  padding: 0.3rem 3rem 3rem 3rem;\n  width: 64rem; }\n\n.modal .title {\n    padding: 1.4rem 0;\n    font-size: 1.8rem;\n    line-height: 3rem; }\n\n.modal .content {\n    display: flex;\n    font-size: 1.4rem; }\n\n.modal .content .message-container {\n      display: flex;\n      flex-direction: column;\n      align-items: flex-start;\n      justify-content: center;\n      margin: 1.4rem 3rem 6.2rem 0; }\n\n.modal .content .message-container .message-block {\n        display: flex;\n        margin-bottom: 1rem; }\n\n.modal .content .message-container .message-block:first-child .message-label {\n          line-height: 4rem; }\n\n.modal .content .message-container .message-block:first-child .message-text {\n          line-height: 4rem; }\n\n.modal .content .message-container .message-block:last-child {\n          margin-bottom: 0; }\n\n.modal .content .message-container .message-block .message-label {\n          min-width: 6.7rem;\n          line-height: 2rem; }\n\n.modal .content .message-container .message-block .message-text {\n          overflow-wrap: break-word;\n          margin-left: 4.8rem;\n          width: 43.4rem;\n          line-height: 2rem; }\n\n.modal .wrapper-buttons {\n    display: flex;\n    align-items: center;\n    justify-content: space-between; }\n\n.modal .wrapper-buttons button {\n      width: 100%;\n      max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9zZW5kLW1vZGFsL0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxzZW5kLW1vZGFsXFxzZW5kLW1vZGFsLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZUFBZTtFQUNmLE1BQU07RUFDTixTQUFTO0VBQ1QsT0FBTztFQUNQLFFBQVE7RUFDUixhQUFhO0VBQ2IsbUJBQW1CO0VBQ25CLHVCQUF1QjtFQUN2QixxQ0FBcUMsRUFBQTs7QUFFdkM7RUFDRSxrQkFBa0I7RUFDbEIsYUFBYTtFQUNiLHNCQUFzQjtFQUN0QiwyQkFBMkI7RUFDM0IscUJBQXFCO0VBQ3JCLDhCQUE4QjtFQUM5QixZQUFZLEVBQUE7O0FBUGQ7SUFVSSxpQkFBaUI7SUFDakIsaUJBQWlCO0lBQ2pCLGlCQUFpQixFQUFBOztBQVpyQjtJQWdCSSxhQUFhO0lBQ2IsaUJBQWlCLEVBQUE7O0FBakJyQjtNQW9CTSxhQUFhO01BQ2Isc0JBQXNCO01BQ3RCLHVCQUF1QjtNQUN2Qix1QkFBdUI7TUFDdkIsNEJBQTRCLEVBQUE7O0FBeEJsQztRQTJCUSxhQUFhO1FBQ2IsbUJBQW1CLEVBQUE7O0FBNUIzQjtVQWlDWSxpQkFBaUIsRUFBQTs7QUFqQzdCO1VBcUNZLGlCQUFpQixFQUFBOztBQXJDN0I7VUEwQ1UsZ0JBQWdCLEVBQUE7O0FBMUMxQjtVQThDVSxpQkFBaUI7VUFDakIsaUJBQWlCLEVBQUE7O0FBL0MzQjtVQW1EVSx5QkFBeUI7VUFDekIsbUJBQW1CO1VBQ25CLGNBQWM7VUFDZCxpQkFBaUIsRUFBQTs7QUF0RDNCO0lBOERJLGFBQWE7SUFDYixtQkFBbUI7SUFDbkIsOEJBQThCLEVBQUE7O0FBaEVsQztNQW1FTSxXQUFXO01BQ1gsZ0JBQWdCLEVBQUEiLCJmaWxlIjoiYXBwL3NlbmQtbW9kYWwvc2VuZC1tb2RhbC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBwb3NpdGlvbjogZml4ZWQ7XHJcbiAgdG9wOiAwO1xyXG4gIGJvdHRvbTogMDtcclxuICBsZWZ0OiAwO1xyXG4gIHJpZ2h0OiAwO1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICBiYWNrZ3JvdW5kOiByZ2JhKDI1NSwgMjU1LCAyNTUsIDAuMjUpO1xyXG59XHJcbi5tb2RhbCB7XHJcbiAgcG9zaXRpb246IHJlbGF0aXZlO1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICBiYWNrZ3JvdW5kLXBvc2l0aW9uOiBjZW50ZXI7XHJcbiAgYmFja2dyb3VuZC1zaXplOiAyMDAlO1xyXG4gIHBhZGRpbmc6IDAuM3JlbSAzcmVtIDNyZW0gM3JlbTtcclxuICB3aWR0aDogNjRyZW07XHJcblxyXG4gIC50aXRsZSB7XHJcbiAgICBwYWRkaW5nOiAxLjRyZW0gMDtcclxuICAgIGZvbnQtc2l6ZTogMS44cmVtO1xyXG4gICAgbGluZS1oZWlnaHQ6IDNyZW07XHJcbiAgfVxyXG5cclxuICAuY29udGVudCB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgZm9udC1zaXplOiAxLjRyZW07XHJcblxyXG4gICAgLm1lc3NhZ2UtY29udGFpbmVyIHtcclxuICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgICAgYWxpZ24taXRlbXM6IGZsZXgtc3RhcnQ7XHJcbiAgICAgIGp1c3RpZnktY29udGVudDogY2VudGVyO1xyXG4gICAgICBtYXJnaW46IDEuNHJlbSAzcmVtIDYuMnJlbSAwO1xyXG4gICAgICBcclxuICAgICAgLm1lc3NhZ2UtYmxvY2sge1xyXG4gICAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgICAgbWFyZ2luLWJvdHRvbTogMXJlbTtcclxuXHJcbiAgICAgICAgJjpmaXJzdC1jaGlsZCB7XHJcblxyXG4gICAgICAgICAgLm1lc3NhZ2UtbGFiZWwge1xyXG4gICAgICAgICAgICBsaW5lLWhlaWdodDogNHJlbTtcclxuICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAubWVzc2FnZS10ZXh0IHtcclxuICAgICAgICAgICAgbGluZS1oZWlnaHQ6IDRyZW07XHJcbiAgICAgICAgICB9XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAmOmxhc3QtY2hpbGQge1xyXG4gICAgICAgICAgbWFyZ2luLWJvdHRvbTogMDtcclxuICAgICAgICB9XHJcblxyXG4gICAgICAgIC5tZXNzYWdlLWxhYmVsIHtcclxuICAgICAgICAgIG1pbi13aWR0aDogNi43cmVtO1xyXG4gICAgICAgICAgbGluZS1oZWlnaHQ6IDJyZW07XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAubWVzc2FnZS10ZXh0IHtcclxuICAgICAgICAgIG92ZXJmbG93LXdyYXA6IGJyZWFrLXdvcmQ7XHJcbiAgICAgICAgICBtYXJnaW4tbGVmdDogNC44cmVtO1xyXG4gICAgICAgICAgd2lkdGg6IDQzLjRyZW07XHJcbiAgICAgICAgICBsaW5lLWhlaWdodDogMnJlbTtcclxuICAgICAgICB9XHJcbiAgICAgIH1cclxuXHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAud3JhcHBlci1idXR0b25zIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAganVzdGlmeS1jb250ZW50OiBzcGFjZS1iZXR3ZWVuO1xyXG5cclxuICAgIGJ1dHRvbiB7XHJcbiAgICAgIHdpZHRoOiAxMDAlO1xyXG4gICAgICBtYXgtd2lkdGg6IDE1cmVtO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbn1cclxuIl19 */"

/***/ }),

/***/ "./src/app/send-modal/send-modal.component.ts":
/*!****************************************************!*\
  !*** ./src/app/send-modal/send-modal.component.ts ***!
  \****************************************************/
/*! exports provided: SendModalComponent */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "SendModalComponent", function() { return SendModalComponent; });
/* harmony import */ var _angular_core__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! @angular/core */ "./node_modules/@angular/core/fesm5/core.js");
/* harmony import */ var _angular_forms__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! @angular/forms */ "./node_modules/@angular/forms/fesm5/forms.js");
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



var SendModalComponent = /** @class */ (function () {
    function SendModalComponent(variablesService) {
        this.variablesService = variablesService;
        this.confirmed = new _angular_core__WEBPACK_IMPORTED_MODULE_0__["EventEmitter"]();
    }
    SendModalComponent.prototype.ngOnInit = function () {
    };
    SendModalComponent.prototype.confirm = function () {
        this.confirmed.emit(true);
    };
    SendModalComponent.prototype.onClose = function () {
        this.confirmed.emit(false);
    };
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Input"])(),
        __metadata("design:type", _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormGroup"])
    ], SendModalComponent.prototype, "form", void 0);
    __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Output"])(),
        __metadata("design:type", _angular_core__WEBPACK_IMPORTED_MODULE_0__["EventEmitter"])
    ], SendModalComponent.prototype, "confirmed", void 0);
    SendModalComponent = __decorate([
        Object(_angular_core__WEBPACK_IMPORTED_MODULE_0__["Component"])({
            selector: 'app-send-modal',
            template: __webpack_require__(/*! ./send-modal.component.html */ "./src/app/send-modal/send-modal.component.html"),
            styles: [__webpack_require__(/*! ./send-modal.component.scss */ "./src/app/send-modal/send-modal.component.scss")]
        }),
        __metadata("design:paramtypes", [_helpers_services_variables_service__WEBPACK_IMPORTED_MODULE_2__["VariablesService"]])
    ], SendModalComponent);
    return SendModalComponent;
}());



/***/ }),

/***/ "./src/app/send/send.component.html":
/*!******************************************!*\
  !*** ./src/app/send/send.component.html ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "<form class=\"form-send\" [formGroup]=\"sendForm\" (ngSubmit)=\"showDialog()\">\r\n\r\n  <div class=\"input-block input-block-alias\">\r\n    <label for=\"send-address\">{{ 'SEND.ADDRESS' | translate }}</label>\r\n\r\n    <input type=\"text\" id=\"send-address\" formControlName=\"address\" (mousedown)=\"addressMouseDown($event)\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n\r\n    <div class=\"alias-dropdown scrolled-content\" *ngIf=\"isOpen\">\r\n      <div *ngFor=\"let item of localAliases\" (click)=\"setAlias(item.name)\">{{item.name}}</div>\r\n    </div>\r\n\r\n    <div class=\"error-block\" *ngIf=\"sendForm.controls['address'].invalid && (sendForm.controls['address'].dirty || sendForm.controls['address'].touched)\">\r\n      <div *ngIf=\"sendForm.controls['address'].errors['required']\">\r\n        {{ 'SEND.FORM_ERRORS.ADDRESS_REQUIRED' | translate }}\r\n      </div>\r\n      <div *ngIf=\"sendForm.controls['address'].errors['address_not_valid']\">\r\n        {{ 'SEND.FORM_ERRORS.ADDRESS_NOT_VALID' | translate }}\r\n      </div>\r\n      <div *ngIf=\"sendForm.controls['address'].errors['alias_not_valid']\">\r\n        {{ 'SEND.FORM_ERRORS.ALIAS_NOT_VALID' | translate }}\r\n      </div>\r\n    </div>\r\n  </div>\r\n\r\n  <div class=\"input-blocks-row\">\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"send-amount\">{{ 'SEND.AMOUNT' | translate }}</label>\r\n      <input type=\"text\" id=\"send-amount\" formControlName=\"amount\" appInputValidate=\"money\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"sendForm.controls['amount'].invalid && (sendForm.controls['amount'].dirty || sendForm.controls['amount'].touched)\">\r\n        <div *ngIf=\"sendForm.controls['amount'].errors['required']\">\r\n          {{ 'SEND.FORM_ERRORS.AMOUNT_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"sendForm.controls['amount'].errors['zero']\">\r\n          {{ 'SEND.FORM_ERRORS.AMOUNT_ZERO' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"send-comment\">{{ 'SEND.COMMENT' | translate }}</label>\r\n      <input type=\"text\" id=\"send-comment\" formControlName=\"comment\" [maxLength]=\"variablesService.maxCommentLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"sendForm.get('comment').value && sendForm.get('comment').value.length >= variablesService.maxCommentLength\">\r\n        {{ 'SEND.FORM_ERRORS.MAX_LENGTH' | translate }}\r\n      </div>\r\n    </div>\r\n\r\n  </div>\r\n\r\n  <button type=\"button\" class=\"send-select\" (click)=\"toggleOptions()\">\r\n    <span>{{ 'SEND.DETAILS' | translate }}</span><i class=\"icon arrow\" [class.down]=\"!additionalOptions\" [class.up]=\"additionalOptions\"></i>\r\n  </button>\r\n\r\n  <div class=\"additional-details\" *ngIf=\"additionalOptions\">\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"send-mixin\">{{ 'SEND.MIXIN' | translate }}</label>\r\n      <input type=\"text\" id=\"send-mixin\" formControlName=\"mixin\" appInputValidate=\"integer\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"sendForm.controls['mixin'].invalid && (sendForm.controls['mixin'].dirty || sendForm.controls['mixin'].touched)\">\r\n        <div *ngIf=\"sendForm.controls['mixin'].errors['required']\">\r\n          {{ 'SEND.FORM_ERRORS.AMOUNT_REQUIRED' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"send-fee\">{{ 'SEND.FEE' | translate }}</label>\r\n      <input type=\"text\" id=\"send-fee\" formControlName=\"fee\" appInputValidate=\"money\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"sendForm.controls['fee'].invalid && (sendForm.controls['fee'].dirty || sendForm.controls['fee'].touched)\">\r\n        <div *ngIf=\"sendForm.controls['fee'].errors['required']\">\r\n          {{ 'SEND.FORM_ERRORS.FEE_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"sendForm.controls['fee'].errors['less_min']\">\r\n          {{ 'SEND.FORM_ERRORS.FEE_MINIMUM' | translate : {fee: variablesService.default_fee} }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"checkbox-block\">\r\n      <input type=\"checkbox\" id=\"send-hide\" class=\"style-checkbox\" formControlName=\"hide\">\r\n      <label for=\"send-hide\">{{ 'SEND.HIDE' | translate }}</label>\r\n    </div>\r\n\r\n  </div>\r\n\r\n  <button type=\"submit\" class=\"blue-button\" [disabled]=\"!sendForm.valid || !variablesService.currentWallet.loaded\">{{ 'SEND.BUTTON' | translate }}</button>\r\n\r\n  <app-send-modal *ngIf=\"isModalDialogVisible\" [form]=\"sendForm\" (confirmed)=\"confirmed($event)\"></app-send-modal>\r\n\r\n</form>\r\n"

/***/ }),

/***/ "./src/app/send/send.component.scss":
/*!******************************************!*\
  !*** ./src/app/send/send.component.scss ***!
  \******************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  width: 100%; }\n\n.form-send .input-blocks-row {\n  display: flex; }\n\n.form-send .input-blocks-row > div {\n    flex-basis: 50%; }\n\n.form-send .input-blocks-row > div:first-child {\n      margin-right: 1.5rem; }\n\n.form-send .input-blocks-row > div:last-child {\n      margin-left: 1.5rem; }\n\n.form-send .send-select {\n  display: flex;\n  align-items: center;\n  background: transparent;\n  border: none;\n  font-size: 1.3rem;\n  line-height: 1.3rem;\n  margin: 1.5rem 0 0;\n  padding: 0;\n  width: 100%;\n  max-width: 15rem;\n  height: 1.3rem; }\n\n.form-send .send-select .arrow {\n    margin-left: 1rem;\n    width: 0.8rem;\n    height: 0.8rem; }\n\n.form-send .send-select .arrow.down {\n      -webkit-mask: url('arrow-down.svg') no-repeat center;\n              mask: url('arrow-down.svg') no-repeat center; }\n\n.form-send .send-select .arrow.up {\n      -webkit-mask: url('arrow-up.svg') no-repeat center;\n              mask: url('arrow-up.svg') no-repeat center; }\n\n.form-send .additional-details {\n  display: flex;\n  margin-top: 1.5rem;\n  padding: 0.5rem 0 2rem; }\n\n.form-send .additional-details > div {\n    flex-basis: 25%; }\n\n.form-send .additional-details > div:first-child {\n      padding-left: 1.5rem;\n      padding-right: 1rem; }\n\n.form-send .additional-details > div:last-child {\n      padding-left: 1rem;\n      padding-right: 1.5rem; }\n\n.form-send .additional-details .checkbox-block {\n    flex-basis: 50%; }\n\n.form-send .additional-details .checkbox-block > label {\n      top: 3.5rem; }\n\n.form-send button {\n  margin: 2.4rem 0;\n  width: 100%;\n  max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9zZW5kL0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxzZW5kXFxzZW5kLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsV0FBVyxFQUFBOztBQUdiO0VBR0ksYUFBYSxFQUFBOztBQUhqQjtJQU1NLGVBQWUsRUFBQTs7QUFOckI7TUFTUSxvQkFBb0IsRUFBQTs7QUFUNUI7TUFhUSxtQkFBbUIsRUFBQTs7QUFiM0I7RUFtQkksYUFBYTtFQUNiLG1CQUFtQjtFQUNuQix1QkFBdUI7RUFDdkIsWUFBWTtFQUNaLGlCQUFpQjtFQUNqQixtQkFBbUI7RUFDbkIsa0JBQWtCO0VBQ2xCLFVBQVU7RUFDVixXQUFXO0VBQ1gsZ0JBQWdCO0VBQ2hCLGNBQWMsRUFBQTs7QUE3QmxCO0lBZ0NNLGlCQUFpQjtJQUNqQixhQUFhO0lBQ2IsY0FBYyxFQUFBOztBQWxDcEI7TUFxQ1Esb0RBQTREO2NBQTVELDRDQUE0RCxFQUFBOztBQXJDcEU7TUF5Q1Esa0RBQTBEO2NBQTFELDBDQUEwRCxFQUFBOztBQXpDbEU7RUErQ0ksYUFBYTtFQUNiLGtCQUFrQjtFQUNsQixzQkFBc0IsRUFBQTs7QUFqRDFCO0lBb0RNLGVBQWUsRUFBQTs7QUFwRHJCO01BdURRLG9CQUFvQjtNQUNwQixtQkFBbUIsRUFBQTs7QUF4RDNCO01BNERRLGtCQUFrQjtNQUNsQixxQkFBcUIsRUFBQTs7QUE3RDdCO0lBa0VNLGVBQWUsRUFBQTs7QUFsRXJCO01BcUVRLFdBQVcsRUFBQTs7QUFyRW5CO0VBMkVJLGdCQUFnQjtFQUNoQixXQUFXO0VBQ1gsZ0JBQWdCLEVBQUEiLCJmaWxlIjoiYXBwL3NlbmQvc2VuZC5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICB3aWR0aDogMTAwJTtcclxufVxyXG5cclxuLmZvcm0tc2VuZCB7XHJcblxyXG4gIC5pbnB1dC1ibG9ja3Mtcm93IHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcblxyXG4gICAgPiBkaXYge1xyXG4gICAgICBmbGV4LWJhc2lzOiA1MCU7XHJcblxyXG4gICAgICAmOmZpcnN0LWNoaWxkIHtcclxuICAgICAgICBtYXJnaW4tcmlnaHQ6IDEuNXJlbTtcclxuICAgICAgfVxyXG5cclxuICAgICAgJjpsYXN0LWNoaWxkIHtcclxuICAgICAgICBtYXJnaW4tbGVmdDogMS41cmVtO1xyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICAuc2VuZC1zZWxlY3Qge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBiYWNrZ3JvdW5kOiB0cmFuc3BhcmVudDtcclxuICAgIGJvcmRlcjogbm9uZTtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbGluZS1oZWlnaHQ6IDEuM3JlbTtcclxuICAgIG1hcmdpbjogMS41cmVtIDAgMDtcclxuICAgIHBhZGRpbmc6IDA7XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIG1heC13aWR0aDogMTVyZW07XHJcbiAgICBoZWlnaHQ6IDEuM3JlbTtcclxuXHJcbiAgICAuYXJyb3cge1xyXG4gICAgICBtYXJnaW4tbGVmdDogMXJlbTtcclxuICAgICAgd2lkdGg6IDAuOHJlbTtcclxuICAgICAgaGVpZ2h0OiAwLjhyZW07XHJcblxyXG4gICAgICAmLmRvd24ge1xyXG4gICAgICAgIG1hc2s6IHVybCh+c3JjL2Fzc2V0cy9pY29ucy9hcnJvdy1kb3duLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi51cCB7XHJcbiAgICAgICAgbWFzazogdXJsKH5zcmMvYXNzZXRzL2ljb25zL2Fycm93LXVwLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLmFkZGl0aW9uYWwtZGV0YWlscyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgbWFyZ2luLXRvcDogMS41cmVtO1xyXG4gICAgcGFkZGluZzogMC41cmVtIDAgMnJlbTtcclxuXHJcbiAgICA+IGRpdiB7XHJcbiAgICAgIGZsZXgtYmFzaXM6IDI1JTtcclxuXHJcbiAgICAgICY6Zmlyc3QtY2hpbGQge1xyXG4gICAgICAgIHBhZGRpbmctbGVmdDogMS41cmVtO1xyXG4gICAgICAgIHBhZGRpbmctcmlnaHQ6IDFyZW07XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICY6bGFzdC1jaGlsZCB7XHJcbiAgICAgICAgcGFkZGluZy1sZWZ0OiAxcmVtO1xyXG4gICAgICAgIHBhZGRpbmctcmlnaHQ6IDEuNXJlbTtcclxuICAgICAgfVxyXG4gICAgfVxyXG5cclxuICAgIC5jaGVja2JveC1ibG9jayB7XHJcbiAgICAgIGZsZXgtYmFzaXM6IDUwJTtcclxuXHJcbiAgICAgID4gbGFiZWwge1xyXG4gICAgICAgIHRvcDogMy41cmVtO1xyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG5cclxuICBidXR0b24ge1xyXG4gICAgbWFyZ2luOiAyLjRyZW0gMDtcclxuICAgIHdpZHRoOiAxMDAlO1xyXG4gICAgbWF4LXdpZHRoOiAxNXJlbTtcclxuICB9XHJcbn1cclxuIl19 */"

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
        this.isModalDialogVisible = false;
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
            comment: new _angular_forms__WEBPACK_IMPORTED_MODULE_1__["FormControl"](''),
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
    SendComponent.prototype.showDialog = function () {
        this.isModalDialogVisible = true;
    };
    SendComponent.prototype.confirmed = function (confirmed) {
        if (confirmed) {
            this.onSend();
        }
        this.isModalDialogVisible = false;
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

module.exports = "<div class=\"content scrolled-content\">\r\n\r\n  <div>\r\n    <div class=\"head\">\r\n      <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n        <i class=\"icon back\"></i>\r\n        <span>{{ 'COMMON.BACK' | translate }}</span>\r\n      </button>\r\n    </div>\r\n\r\n    <h3 class=\"settings-title\">{{ 'SETTINGS.TITLE' | translate }}</h3>\r\n\r\n    <div class=\"theme-selection\">\r\n      <div class=\"radio-block\">\r\n        <input class=\"style-radio\" type=\"radio\" id=\"dark\" name=\"theme\" value=\"dark\" [checked]=\"theme == 'dark'\" (change)=\"setTheme('dark')\">\r\n        <label for=\"dark\">{{ 'SETTINGS.DARK_THEME' | translate }}</label>\r\n      </div>\r\n      <div class=\"radio-block\">\r\n        <input class=\"style-radio\" type=\"radio\" id=\"white\" name=\"theme\" value=\"white\" [checked]=\"theme == 'white'\" (change)=\"setTheme('white')\">\r\n        <label for=\"white\">{{ 'SETTINGS.WHITE_THEME' | translate }}</label>\r\n      </div>\r\n      <div class=\"radio-block\">\r\n        <input class=\"style-radio\" type=\"radio\" id=\"gray\" name=\"theme\" value=\"gray\" [checked]=\"theme == 'gray'\" (change)=\"setTheme('gray')\">\r\n        <label for=\"gray\">{{ 'SETTINGS.GRAY_THEME' | translate }}</label>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"scale-selection\">\r\n      <button type=\"button\" class=\"button-block\" [class.active]=\"item.id === variablesService.settings.scale\" *ngFor=\"let item of appScaleOptions\" (click)=\"setScale(item.id)\">\r\n        <span class=\"label\">{{item.name}}</span>\r\n      </button>\r\n    </div>\r\n\r\n    <div class=\"lock-selection\">\r\n      <label class=\"lock-selection-title\">{{ 'SETTINGS.APP_LOCK.TITLE' | translate }}</label>\r\n      <ng-select class=\"custom-select\"\r\n                 [items]=\"appLockOptions\"\r\n                 bindValue=\"id\"\r\n                 bindLabel=\"name\"\r\n                 [(ngModel)]=\"variablesService.settings.appLockTime\"\r\n                 [clearable]=\"false\"\r\n                 [searchable]=\"false\"\r\n                 (change)=\"onLockChange()\">\r\n        <ng-template ng-label-tmp let-item=\"item\">\r\n          {{item.name | translate}}\r\n        </ng-template>\r\n        <ng-template ng-option-tmp let-item=\"item\" let-index=\"index\">\r\n          {{item.name | translate}}\r\n        </ng-template>\r\n      </ng-select>\r\n    </div>\r\n\r\n    <div class=\"lock-selection\">\r\n      <label class=\"lock-selection-title\">{{ 'SETTINGS.APP_LOG_TITLE' | translate }}</label>\r\n      <ng-select class=\"custom-select\"\r\n                 [items]=\"appLogOptions\"\r\n                 bindValue=\"id\"\r\n                 bindLabel=\"id\"\r\n                 [(ngModel)]=\"variablesService.settings.appLog\"\r\n                 [clearable]=\"false\"\r\n                 [searchable]=\"false\"\r\n                 (change)=\"onLogChange()\">\r\n      </ng-select>\r\n    </div>\r\n\r\n    <form class=\"master-password\" [formGroup]=\"changeForm\" (ngSubmit)=\"onSubmitChangePass()\">\r\n\r\n      <span class=\"master-password-title\">{{ 'SETTINGS.MASTER_PASSWORD.TITLE' | translate }}</span>\r\n\r\n      <div class=\"input-block\" *ngIf=\"variablesService.appPass\">\r\n        <label for=\"old-password\">{{ 'SETTINGS.MASTER_PASSWORD.OLD' | translate }}</label>\r\n        <input type=\"password\" id=\"old-password\" formControlName=\"password\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\"/>\r\n        <div class=\"error-block\" *ngIf=\"changeForm.invalid && changeForm.controls['password'].valid && (changeForm.controls['password'].dirty || changeForm.controls['password'].touched) && changeForm.errors && changeForm.errors.pass_mismatch\">\r\n          {{ 'SETTINGS.FORM_ERRORS.PASS_NOT_MATCH' | translate }}\r\n        </div>\r\n      </div>\r\n\r\n      <div class=\"input-block\">\r\n        <label for=\"new-password\">{{ 'SETTINGS.MASTER_PASSWORD.NEW' | translate }}</label>\r\n        <input type=\"password\" id=\"new-password\" formControlName=\"new_password\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\"/>\r\n      </div>\r\n\r\n      <div class=\"input-block\">\r\n        <label for=\"confirm-password\">{{ 'SETTINGS.MASTER_PASSWORD.CONFIRM' | translate }}</label>\r\n        <input type=\"password\" id=\"confirm-password\" formControlName=\"new_confirmation\" (contextmenu)=\"variablesService.onContextMenuPasteSelect($event)\"/>\r\n        <div class=\"error-block\" *ngIf=\"changeForm.invalid && (changeForm.controls['new_confirmation'].dirty || changeForm.controls['new_confirmation'].touched) && changeForm.errors && changeForm.errors.confirm_mismatch\">\r\n          {{ 'SETTINGS.FORM_ERRORS.CONFIRM_NOT_MATCH' | translate }}\r\n        </div>\r\n      </div>\r\n\r\n      <button type=\"submit\" class=\"blue-button\" [disabled]=\"!changeForm.valid\">{{ 'SETTINGS.MASTER_PASSWORD.BUTTON' | translate }}</button>\r\n\r\n    </form>\r\n  </div>\r\n\r\n  <div>\r\n    <div class=\"last-build\">{{ 'SETTINGS.LAST_BUILD' | translate : {value: currentBuild} }}</div>\r\n  </div>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/settings/settings.component.scss":
/*!**************************************************!*\
  !*** ./src/app/settings/settings.component.scss ***!
  \**************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".head {\n  justify-content: flex-end; }\n\n.settings-title {\n  font-size: 1.7rem; }\n\n.theme-selection {\n  display: flex;\n  flex-direction: column;\n  align-items: flex-start;\n  margin: 2.4rem 0;\n  width: 50%; }\n\n.theme-selection .radio-block {\n    display: flex;\n    align-items: center;\n    justify-content: flex-start;\n    font-size: 1.3rem;\n    line-height: 2.7rem; }\n\n.lock-selection {\n  display: flex;\n  flex-direction: column;\n  align-items: flex-start;\n  margin: 2.4rem 0;\n  width: 50%; }\n\n.lock-selection .lock-selection-title {\n    display: flex;\n    font-size: 1.5rem;\n    line-height: 2.7rem;\n    margin-bottom: 1rem; }\n\n.scale-selection {\n  display: flex;\n  align-items: center;\n  justify-content: space-between;\n  padding: 0 0 4rem;\n  width: 50%;\n  height: 0.5rem; }\n\n.scale-selection .button-block {\n    position: relative;\n    display: flex;\n    align-items: center;\n    justify-content: center;\n    flex: 1 0 auto;\n    margin: 0 0.2rem;\n    padding: 0;\n    height: 0.5rem; }\n\n.scale-selection .button-block .label {\n      position: absolute;\n      bottom: -1rem;\n      left: 50%;\n      transform: translate(-50%, 100%);\n      font-size: 1rem;\n      white-space: nowrap; }\n\n.master-password {\n  width: 50%; }\n\n.master-password .master-password-title {\n    display: flex;\n    font-size: 1.5rem;\n    line-height: 2.7rem;\n    margin-bottom: 1rem; }\n\n.master-password button {\n    margin: 2.5rem auto;\n    width: 100%;\n    max-width: 15rem; }\n\n.last-build {\n  font-size: 1rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9zZXR0aW5ncy9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcc2V0dGluZ3NcXHNldHRpbmdzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UseUJBQXlCLEVBQUE7O0FBRzNCO0VBQ0UsaUJBQWlCLEVBQUE7O0FBR25CO0VBQ0UsYUFBYTtFQUNiLHNCQUFzQjtFQUN0Qix1QkFBdUI7RUFDdkIsZ0JBQWdCO0VBQ2hCLFVBQVUsRUFBQTs7QUFMWjtJQVFJLGFBQWE7SUFDYixtQkFBbUI7SUFDbkIsMkJBQTJCO0lBQzNCLGlCQUFpQjtJQUNqQixtQkFBbUIsRUFBQTs7QUFJdkI7RUFDRSxhQUFhO0VBQ2Isc0JBQXNCO0VBQ3RCLHVCQUF1QjtFQUN2QixnQkFBZ0I7RUFDaEIsVUFBVSxFQUFBOztBQUxaO0lBUUksYUFBYTtJQUNiLGlCQUFpQjtJQUNqQixtQkFBbUI7SUFDbkIsbUJBQW1CLEVBQUE7O0FBSXZCO0VBQ0UsYUFBYTtFQUNiLG1CQUFtQjtFQUNuQiw4QkFBOEI7RUFDOUIsaUJBQWlCO0VBQ2pCLFVBQVU7RUFDVixjQUFjLEVBQUE7O0FBTmhCO0lBU0ksa0JBQWtCO0lBQ2xCLGFBQWE7SUFDYixtQkFBbUI7SUFDbkIsdUJBQXVCO0lBQ3ZCLGNBQWM7SUFDZCxnQkFBZ0I7SUFDaEIsVUFBVTtJQUNWLGNBQWMsRUFBQTs7QUFoQmxCO01BbUJNLGtCQUFrQjtNQUNsQixhQUFhO01BQ2IsU0FBUztNQUNULGdDQUFnQztNQUNoQyxlQUFlO01BQ2YsbUJBQW1CLEVBQUE7O0FBS3pCO0VBQ0UsVUFBVSxFQUFBOztBQURaO0lBSUksYUFBYTtJQUNiLGlCQUFpQjtJQUNqQixtQkFBbUI7SUFDbkIsbUJBQW1CLEVBQUE7O0FBUHZCO0lBV0ksbUJBQW1CO0lBQ25CLFdBQVc7SUFDWCxnQkFBZ0IsRUFBQTs7QUFJcEI7RUFDRSxlQUFlLEVBQUEiLCJmaWxlIjoiYXBwL3NldHRpbmdzL3NldHRpbmdzLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmhlYWQge1xyXG4gIGp1c3RpZnktY29udGVudDogZmxleC1lbmQ7XHJcbn1cclxuXHJcbi5zZXR0aW5ncy10aXRsZSB7XHJcbiAgZm9udC1zaXplOiAxLjdyZW07XHJcbn1cclxuXHJcbi50aGVtZS1zZWxlY3Rpb24ge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICBhbGlnbi1pdGVtczogZmxleC1zdGFydDtcclxuICBtYXJnaW46IDIuNHJlbSAwO1xyXG4gIHdpZHRoOiA1MCU7XHJcblxyXG4gIC5yYWRpby1ibG9jayB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGp1c3RpZnktY29udGVudDogZmxleC1zdGFydDtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbGluZS1oZWlnaHQ6IDIuN3JlbTtcclxuICB9XHJcbn1cclxuXHJcbi5sb2NrLXNlbGVjdGlvbiB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gIGFsaWduLWl0ZW1zOiBmbGV4LXN0YXJ0O1xyXG4gIG1hcmdpbjogMi40cmVtIDA7XHJcbiAgd2lkdGg6IDUwJTtcclxuXHJcbiAgLmxvY2stc2VsZWN0aW9uLXRpdGxlIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmb250LXNpemU6IDEuNXJlbTtcclxuICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XHJcbiAgICBtYXJnaW4tYm90dG9tOiAxcmVtO1xyXG4gIH1cclxufVxyXG5cclxuLnNjYWxlLXNlbGVjdGlvbiB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICBwYWRkaW5nOiAwIDAgNHJlbTtcclxuICB3aWR0aDogNTAlO1xyXG4gIGhlaWdodDogMC41cmVtO1xyXG5cclxuICAuYnV0dG9uLWJsb2NrIHtcclxuICAgIHBvc2l0aW9uOiByZWxhdGl2ZTtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XHJcbiAgICBmbGV4OiAxIDAgYXV0bztcclxuICAgIG1hcmdpbjogMCAwLjJyZW07XHJcbiAgICBwYWRkaW5nOiAwO1xyXG4gICAgaGVpZ2h0OiAwLjVyZW07XHJcblxyXG4gICAgLmxhYmVsIHtcclxuICAgICAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gICAgICBib3R0b206IC0xcmVtO1xyXG4gICAgICBsZWZ0OiA1MCU7XHJcbiAgICAgIHRyYW5zZm9ybTogdHJhbnNsYXRlKC01MCUsIDEwMCUpO1xyXG4gICAgICBmb250LXNpemU6IDFyZW07XHJcbiAgICAgIHdoaXRlLXNwYWNlOiBub3dyYXA7XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcblxyXG4ubWFzdGVyLXBhc3N3b3JkIHtcclxuICB3aWR0aDogNTAlO1xyXG5cclxuICAubWFzdGVyLXBhc3N3b3JkLXRpdGxlIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmb250LXNpemU6IDEuNXJlbTtcclxuICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XHJcbiAgICBtYXJnaW4tYm90dG9tOiAxcmVtO1xyXG4gIH1cclxuXHJcbiAgYnV0dG9uIHtcclxuICAgIG1hcmdpbjogMi41cmVtIGF1dG87XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIG1heC13aWR0aDogMTVyZW07XHJcbiAgfVxyXG59XHJcblxyXG4ubGFzdC1idWlsZCB7XHJcbiAgZm9udC1zaXplOiAxcmVtO1xyXG59XHJcbiJdfQ== */"

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
        var _this = this;
        if (this.changeForm.valid) {
            this.variablesService.appPass = this.changeForm.get('new_password').value;
            if (this.variablesService.appPass) {
                this.backend.setMasterPassword({ pass: this.variablesService.appPass }, function (status, data) {
                    if (status) {
                        _this.backend.storeSecureAppData({ pass: _this.variablesService.appPass });
                        _this.variablesService.appLogin = true;
                        _this.variablesService.dataIsLoaded = true;
                        _this.variablesService.startCountdown();
                    }
                    else {
                        console.log(data['error_code']);
                    }
                });
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

module.exports = "<div class=\"sidebar-accounts\">\r\n  <div class=\"sidebar-accounts-header\">\r\n    <h3>{{ 'SIDEBAR.TITLE' | translate }}</h3><button [routerLink]=\"['main']\">{{ 'SIDEBAR.ADD_NEW' | translate }}</button>\r\n  </div>\r\n  <div class=\"sidebar-accounts-list scrolled-content\">\r\n    <div class=\"sidebar-account\" *ngFor=\"let wallet of variablesService.wallets\" [class.active]=\"wallet?.wallet_id === walletActive\" [routerLink]=\"['/wallet/' + wallet.wallet_id + '/history']\">\r\n      <div class=\"sidebar-account-row account-title-balance\">\r\n        <span class=\"title\" tooltip=\"{{ wallet.name }}\" placement=\"top-left\" tooltipClass=\"table-tooltip account-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{wallet.name}}</span>\r\n        <span class=\"balance\">{{wallet.balance | intToMoney : '3' }} {{variablesService.defaultCurrency}}</span>\r\n      </div>\r\n      <div class=\"sidebar-account-row account-alias\">\r\n        <div class=\"name\">\r\n          <span tooltip=\"{{wallet.alias['name']}}\" placement=\"top-left\" tooltipClass=\"table-tooltip account-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{wallet.alias['name']}}</span>\r\n          <ng-container *ngIf=\"wallet.alias['comment'] && wallet.alias['comment'].length\">\r\n            <i class=\"icon comment\" tooltip=\"{{wallet.alias['comment']}}\" placement=\"top\" tooltipClass=\"table-tooltip account-tooltip\" [delay]=\"500\"></i>\r\n          </ng-container>\r\n        </div>\r\n        <span class=\"price\">$ {{wallet.getMoneyEquivalent(variablesService.moneyEquivalent) | intToMoney | number : '1.2-2'}}</span>\r\n      </div>\r\n      <div class=\"sidebar-account-row account-staking\" *ngIf=\"!(!wallet.loaded && variablesService.daemon_state === 2)\">\r\n        <span class=\"text\">{{ 'SIDEBAR.ACCOUNT.STAKING' | translate }}</span>\r\n        <app-staking-switch [wallet_id]=\"wallet.wallet_id\" [(staking)]=\"wallet.staking\"></app-staking-switch>\r\n      </div>\r\n      <div class=\"sidebar-account-row account-messages\" *ngIf=\"!(!wallet.loaded && variablesService.daemon_state === 2)\">\r\n        <span class=\"text\">{{ 'SIDEBAR.ACCOUNT.MESSAGES' | translate }}</span>\r\n        <span class=\"indicator\">{{wallet.new_contracts}}</span>\r\n      </div>\r\n      <div class=\"sidebar-account-row account-synchronization\" *ngIf=\"!wallet.loaded && variablesService.daemon_state === 2\">\r\n        <span class=\"status\">{{ 'SIDEBAR.ACCOUNT.SYNCING' | translate }}</span>\r\n        <div class=\"progress-bar-container\">\r\n          <div class=\"progress-bar\">\r\n            <div class=\"fill\" [style.width]=\"wallet.progress + '%'\"></div>\r\n          </div>\r\n          <div class=\"progress-percent\">{{ wallet.progress }}%</div>\r\n        </div>\r\n      </div>\r\n    </div>\r\n  </div>\r\n</div>\r\n<div class=\"sidebar-settings\">\r\n  <div class=\"wrap-button\" routerLinkActive=\"active\">\r\n    <button (click)=\"contactsRoute()\" [class.disabled]=\"variablesService.daemon_state !== 2\" [disabled]=\"variablesService.daemon_state !== 2\">\r\n      <i class=\"icon contacts\"></i>\r\n      <span>{{ 'SIDEBAR.CONTACTS' | translate }}</span>\r\n    </button>\r\n  </div>\r\n  <div class=\"wrap-button\" routerLinkActive=\"active\">\r\n    <button [routerLink]=\"['/settings']\">\r\n      <i class=\"icon settings\"></i>\r\n      <span>{{ 'SIDEBAR.SETTINGS' | translate }}</span>\r\n    </button>\r\n  </div>\r\n  <div class=\"wrap-button\">\r\n    <button (click)=\"logOut()\">\r\n      <i class=\"icon logout\"></i>\r\n      <span>{{ 'SIDEBAR.LOG_OUT' | translate }}</span>\r\n    </button>\r\n  </div>\r\n</div>\r\n<div class=\"sidebar-synchronization-status\">\r\n  <div class=\"status-container\">\r\n    <span class=\"offline\" *ngIf=\"variablesService.daemon_state === 0\">\r\n      {{ 'SIDEBAR.SYNCHRONIZATION.OFFLINE' | translate }}\r\n    </span>\r\n    <span class=\"syncing\" *ngIf=\"variablesService.daemon_state === 1\">\r\n      {{ 'SIDEBAR.SYNCHRONIZATION.SYNCING' | translate }}\r\n    </span>\r\n    <span class=\"online\" *ngIf=\"variablesService.daemon_state === 2\">\r\n      {{ 'SIDEBAR.SYNCHRONIZATION.ONLINE' | translate }}\r\n    </span>\r\n    <span class=\"loading\" *ngIf=\"variablesService.daemon_state === 3\">\r\n      {{ 'SIDEBAR.SYNCHRONIZATION.LOADING' | translate }}\r\n    </span>\r\n    <span class=\"offline\" *ngIf=\"variablesService.daemon_state === 4\">\r\n      {{ 'SIDEBAR.SYNCHRONIZATION.ERROR' | translate }}\r\n    </span>\r\n    <span class=\"online\" *ngIf=\"variablesService.daemon_state === 5\">\r\n      {{ 'SIDEBAR.SYNCHRONIZATION.COMPLETE' | translate }}\r\n    </span>\r\n    <div class=\"progress-bar-container\" *ngIf=\"variablesService.daemon_state === 1 || variablesService.daemon_state === 3\">\r\n      <div class=\"syncing\" *ngIf=\"variablesService.daemon_state === 1\">\r\n        <div class=\"progress-bar\">\r\n          <div class=\"fill\" [style.width]=\"variablesService.sync.progress_value + '%'\"></div>\r\n        </div>\r\n        <div class=\"progress-percent\">{{ variablesService.sync.progress_value_text }}%</div>\r\n      </div>\r\n      <div class=\"loading\" *ngIf=\"variablesService.daemon_state === 3\"></div>\r\n    </div>\r\n  </div>\r\n  <div class=\"update-container\" *ngIf=\"(variablesService.daemon_state === 0 || variablesService.daemon_state === 2) && [2, 3, 4].indexOf(variablesService.last_build_displaymode) !== -1\">\r\n    <ng-container *ngIf=\"variablesService.last_build_displaymode === 2\">\r\n      <div class=\"update-text standard\">\r\n        <span [style.cursor]=\"'pointer'\" (click)=\"getUpdate()\">{{ 'SIDEBAR.UPDATE.STANDARD' | translate }}</span>\r\n      </div>\r\n      <i class=\"icon update standard\" tooltip=\"{{ 'SIDEBAR.UPDATE.STANDARD_TOOLTIP' | translate }}\" placement=\"right-bottom\" tooltipClass=\"update-tooltip\" [delay]=\"500\"></i>\r\n    </ng-container>\r\n    <ng-container *ngIf=\"variablesService.last_build_displaymode === 3\">\r\n      <div class=\"update-text important\">\r\n        <span [style.cursor]=\"'pointer'\" (click)=\"getUpdate()\">{{ 'SIDEBAR.UPDATE.IMPORTANT' | translate }}</span>\r\n        <br>\r\n        <span style=\"font-size: 1rem\">{{ 'SIDEBAR.UPDATE.IMPORTANT_HINT' | translate }}</span>\r\n      </div>\r\n      <i class=\"icon update important\" tooltip=\"{{ 'SIDEBAR.UPDATE.IMPORTANT_TOOLTIP' | translate }}\" placement=\"right-bottom\" tooltipClass=\"update-tooltip important\" [delay]=\"500\"></i>\r\n    </ng-container>\r\n    <ng-container *ngIf=\"variablesService.last_build_displaymode === 4\">\r\n      <div class=\"update-text critical\">\r\n        <span [style.cursor]=\"'pointer'\" (click)=\"getUpdate()\">{{ 'SIDEBAR.UPDATE.CRITICAL' | translate }}</span>\r\n        <br>\r\n        <span style=\"font-size: 1rem\">{{ 'SIDEBAR.UPDATE.IMPORTANT_HINT' | translate }}</span>\r\n      </div>\r\n      <i class=\"icon update critical\" tooltip=\"{{ 'SIDEBAR.UPDATE.CRITICAL_TOOLTIP' | translate }}\" placement=\"right-bottom\" tooltipClass=\"update-tooltip critical\" [delay]=\"500\"></i>\r\n    </ng-container>\r\n  </div>\r\n  <div class=\"update-container\" *ngIf=\"variablesService.daemon_state === 2 && variablesService.net_time_delta_median !== 0\">\r\n    <div class=\"update-text time\">\r\n      <span>{{ 'SIDEBAR.UPDATE.TIME' | translate }}</span>\r\n    </div>\r\n    <i class=\"icon time\" tooltip=\"{{ 'SIDEBAR.UPDATE.TIME_TOOLTIP' | translate }}\" placement=\"right-bottom\" tooltipClass=\"update-tooltip important\" [delay]=\"500\"></i>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/sidebar/sidebar.component.scss":
/*!************************************************!*\
  !*** ./src/app/sidebar/sidebar.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: flex;\n  flex-direction: column;\n  justify-content: space-between;\n  flex: 0 0 25rem;\n  padding: 0 3rem;\n  max-width: 25rem; }\n\n.sidebar-accounts {\n  position: relative;\n  display: flex;\n  flex-direction: column;\n  flex: 1 1 auto; }\n\n.sidebar-accounts .sidebar-accounts-header {\n    display: flex;\n    align-items: center;\n    justify-content: space-between;\n    flex: 0 0 auto;\n    height: 8rem;\n    font-weight: 400; }\n\n.sidebar-accounts .sidebar-accounts-header h3 {\n      font-size: 1.7rem; }\n\n.sidebar-accounts .sidebar-accounts-header button {\n      background: transparent;\n      border: none;\n      outline: none; }\n\n.sidebar-accounts .sidebar-accounts-list {\n    display: flex;\n    flex-direction: column;\n    flex: 1 1 auto;\n    margin: 0 -3rem;\n    overflow-y: overlay; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account {\n      display: flex;\n      flex-direction: column;\n      flex-shrink: 0;\n      cursor: pointer;\n      padding: 2rem 3rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row {\n        display: flex;\n        align-items: center;\n        justify-content: space-between; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-title-balance {\n          line-height: 2.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-title-balance .title {\n            font-size: 1.5rem;\n            text-overflow: ellipsis;\n            overflow: hidden;\n            white-space: nowrap; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-title-balance .balance {\n            font-size: 1.8rem;\n            font-weight: 600;\n            white-space: nowrap; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias {\n          font-size: 1.3rem;\n          line-height: 3.4rem;\n          margin-bottom: 0.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .name {\n            display: flex;\n            align-items: center;\n            flex-shrink: 1;\n            line-height: 1.6rem;\n            padding-right: 1rem;\n            overflow: hidden; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .name span {\n              text-overflow: ellipsis;\n              overflow: hidden;\n              white-space: nowrap; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .price {\n            flex-shrink: 0; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .icon {\n            margin-left: 0.5rem;\n            width: 1.2rem;\n            height: 1.2rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-alias .icon.comment {\n              -webkit-mask: url('alert.svg') no-repeat center;\n                      mask: url('alert.svg') no-repeat center; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-staking {\n          line-height: 2.9rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-staking .text {\n            font-size: 1.3rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-messages {\n          line-height: 2.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-messages .text {\n            font-size: 1.3rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-messages .indicator {\n            display: flex;\n            align-items: center;\n            justify-content: center;\n            border-radius: 1rem;\n            font-size: 1rem;\n            min-width: 2.4rem;\n            height: 1.6rem;\n            padding: 0 0.5rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization {\n          flex-direction: column;\n          height: 5.6rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .status {\n            align-self: flex-start;\n            font-size: 1.3rem;\n            line-height: 2.6rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container {\n            display: flex;\n            margin: 0.4rem 0;\n            height: 0.7rem;\n            width: 100%; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container .progress-bar {\n              flex: 1 0 auto; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container .progress-bar .fill {\n                height: 100%; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account .sidebar-account-row.account-synchronization .progress-bar-container .progress-percent {\n              flex: 0 0 auto;\n              font-size: 1.3rem;\n              line-height: 0.7rem;\n              padding-left: 0.7rem; }\n\n.sidebar-accounts .sidebar-accounts-list .sidebar-account:focus {\n        outline: none; }\n\n.sidebar-accounts:after {\n    content: '';\n    position: absolute;\n    bottom: 0;\n    left: -3rem;\n    width: calc(100% + 6rem);\n    height: 5rem; }\n\n.sidebar-settings {\n  flex: 0 0 auto;\n  padding-bottom: 1rem; }\n\n.sidebar-settings .wrap-button {\n    margin: 0 -3rem; }\n\n.sidebar-settings .wrap-button button {\n      display: flex;\n      align-items: center;\n      background: transparent;\n      border: none;\n      font-weight: 400;\n      line-height: 3rem;\n      outline: none;\n      padding: 0 3rem;\n      width: 100%; }\n\n.sidebar-settings .wrap-button button.disabled {\n        cursor: url('not-allowed.svg'), not-allowed; }\n\n.sidebar-settings .wrap-button button .icon {\n        margin-right: 1.2rem;\n        width: 1.7rem;\n        height: 1.7rem; }\n\n.sidebar-settings .wrap-button button .icon.contacts {\n          -webkit-mask: url('contacts.svg') no-repeat center;\n                  mask: url('contacts.svg') no-repeat center; }\n\n.sidebar-settings .wrap-button button .icon.settings {\n          -webkit-mask: url('settings.svg') no-repeat center;\n                  mask: url('settings.svg') no-repeat center; }\n\n.sidebar-settings .wrap-button button .icon.logout {\n          -webkit-mask: url('logout.svg') no-repeat center;\n                  mask: url('logout.svg') no-repeat center; }\n\n.sidebar-synchronization-status {\n  display: flex;\n  align-items: center;\n  justify-content: flex-start;\n  flex: 0 0 7rem;\n  font-size: 1.3rem; }\n\n.sidebar-synchronization-status .status-container {\n    position: relative;\n    flex-grow: 1;\n    text-align: left; }\n\n.sidebar-synchronization-status .status-container .offline, .sidebar-synchronization-status .status-container .online {\n      position: relative;\n      display: block;\n      line-height: 1.2rem;\n      padding-left: 2.2rem; }\n\n.sidebar-synchronization-status .status-container .offline:before, .sidebar-synchronization-status .status-container .online:before {\n        content: '';\n        position: absolute;\n        top: 0;\n        left: 0;\n        border-radius: 50%;\n        width: 1.2rem;\n        height: 1.2rem; }\n\n.sidebar-synchronization-status .status-container .syncing, .sidebar-synchronization-status .status-container .loading {\n      line-height: 4rem; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container {\n      position: absolute;\n      bottom: 0;\n      left: 0;\n      height: 0.7rem;\n      width: 100%; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .syncing {\n        display: flex; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .syncing .progress-bar {\n          flex: 1 0 auto; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .syncing .progress-bar .fill {\n            height: 100%; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .syncing .progress-percent {\n          flex: 0 0 auto;\n          font-size: 1.3rem;\n          line-height: 0.7rem;\n          padding-left: 0.7rem; }\n\n.sidebar-synchronization-status .status-container .progress-bar-container .loading {\n        -webkit-animation: move 5s linear infinite;\n                animation: move 5s linear infinite;\n        background-image: -webkit-gradient(linear, 0 0, 100% 100%, color-stop(0.125, rgba(0, 0, 0, 0.15)), color-stop(0.125, transparent), color-stop(0.25, transparent), color-stop(0.25, rgba(0, 0, 0, 0.1)), color-stop(0.375, rgba(0, 0, 0, 0.1)), color-stop(0.375, transparent), color-stop(0.5, transparent), color-stop(0.5, rgba(0, 0, 0, 0.15)), color-stop(0.625, rgba(0, 0, 0, 0.15)), color-stop(0.625, transparent), color-stop(0.75, transparent), color-stop(0.75, rgba(0, 0, 0, 0.1)), color-stop(0.875, rgba(0, 0, 0, 0.1)), color-stop(0.875, transparent), to(transparent)), -webkit-gradient(linear, 0 100%, 100% 0, color-stop(0.125, rgba(0, 0, 0, 0.3)), color-stop(0.125, transparent), color-stop(0.25, transparent), color-stop(0.25, rgba(0, 0, 0, 0.25)), color-stop(0.375, rgba(0, 0, 0, 0.25)), color-stop(0.375, transparent), color-stop(0.5, transparent), color-stop(0.5, rgba(0, 0, 0, 0.3)), color-stop(0.625, rgba(0, 0, 0, 0.3)), color-stop(0.625, transparent), color-stop(0.75, transparent), color-stop(0.75, rgba(0, 0, 0, 0.25)), color-stop(0.875, rgba(0, 0, 0, 0.25)), color-stop(0.875, transparent), to(transparent));\n        background-size: 7rem 7rem;\n        height: 100%; }\n\n.sidebar-synchronization-status .update-container {\n    display: flex;\n    flex-grow: 1;\n    margin-left: 1rem;\n    text-align: right; }\n\n.sidebar-synchronization-status .update-container .update-text {\n      flex: 1 1 auto;\n      font-size: 1.2rem;\n      line-height: 1.8rem;\n      text-align: left; }\n\n.sidebar-synchronization-status .update-container .update-text.time {\n        font-size: 1.1rem; }\n\n.sidebar-synchronization-status .update-container .icon {\n      flex: 1 0 auto;\n      margin: 0.3rem 0 0 0.6rem;\n      width: 1.2rem;\n      height: 1.2rem; }\n\n.sidebar-synchronization-status .update-container .icon.update {\n        -webkit-mask: url('update.svg') no-repeat center;\n                mask: url('update.svg') no-repeat center; }\n\n.sidebar-synchronization-status .update-container .icon.time {\n        -webkit-mask: url('time.svg') no-repeat center;\n                mask: url('time.svg') no-repeat center; }\n\n@-webkit-keyframes move {\n  0% {\n    background-position: 100% -7rem; }\n  100% {\n    background-position: 100% 7rem; } }\n\n@keyframes move {\n  0% {\n    background-position: 100% -7rem; }\n  100% {\n    background-position: 100% 7rem; } }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9zaWRlYmFyL0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxzaWRlYmFyXFxzaWRlYmFyLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsYUFBYTtFQUNiLHNCQUFzQjtFQUN0Qiw4QkFBOEI7RUFDOUIsZUFBZTtFQUNmLGVBQWU7RUFDZixnQkFBZ0IsRUFBQTs7QUFHbEI7RUFDRSxrQkFBa0I7RUFDbEIsYUFBYTtFQUNiLHNCQUFzQjtFQUN0QixjQUFjLEVBQUE7O0FBSmhCO0lBT0ksYUFBYTtJQUNiLG1CQUFtQjtJQUNuQiw4QkFBOEI7SUFDOUIsY0FBYztJQUNkLFlBQVk7SUFDWixnQkFBZ0IsRUFBQTs7QUFacEI7TUFlTSxpQkFBaUIsRUFBQTs7QUFmdkI7TUFtQk0sdUJBQXVCO01BQ3ZCLFlBQVk7TUFDWixhQUFhLEVBQUE7O0FBckJuQjtJQTBCSSxhQUFhO0lBQ2Isc0JBQXNCO0lBQ3RCLGNBQWM7SUFDZCxlQUFlO0lBQ2YsbUJBQW1CLEVBQUE7O0FBOUJ2QjtNQWlDTSxhQUFhO01BQ2Isc0JBQXNCO01BQ3RCLGNBQWM7TUFDZCxlQUFlO01BQ2Ysa0JBQWtCLEVBQUE7O0FBckN4QjtRQXdDUSxhQUFhO1FBQ2IsbUJBQW1CO1FBQ25CLDhCQUE4QixFQUFBOztBQTFDdEM7VUE2Q1UsbUJBQW1CLEVBQUE7O0FBN0M3QjtZQWdEWSxpQkFBaUI7WUFDakIsdUJBQXVCO1lBQ3ZCLGdCQUFnQjtZQUNoQixtQkFBbUIsRUFBQTs7QUFuRC9CO1lBdURZLGlCQUFpQjtZQUNqQixnQkFBZ0I7WUFDaEIsbUJBQW1CLEVBQUE7O0FBekQvQjtVQThEVSxpQkFBaUI7VUFDakIsbUJBQW1CO1VBQ25CLHFCQUFxQixFQUFBOztBQWhFL0I7WUFtRVksYUFBYTtZQUNiLG1CQUFtQjtZQUNuQixjQUFjO1lBQ2QsbUJBQW1CO1lBQ25CLG1CQUFtQjtZQUNuQixnQkFBZ0IsRUFBQTs7QUF4RTVCO2NBMkVjLHVCQUF1QjtjQUN2QixnQkFBZ0I7Y0FDaEIsbUJBQW1CLEVBQUE7O0FBN0VqQztZQWtGWSxjQUFjLEVBQUE7O0FBbEYxQjtZQXNGWSxtQkFBbUI7WUFDbkIsYUFBYTtZQUNiLGNBQWMsRUFBQTs7QUF4RjFCO2NBMkZjLCtDQUF3RDtzQkFBeEQsdUNBQXdELEVBQUE7O0FBM0Z0RTtVQWlHVSxtQkFBbUIsRUFBQTs7QUFqRzdCO1lBb0dZLGlCQUFpQixFQUFBOztBQXBHN0I7VUF5R1UsbUJBQW1CLEVBQUE7O0FBekc3QjtZQTRHWSxpQkFBaUIsRUFBQTs7QUE1RzdCO1lBZ0hZLGFBQWE7WUFDYixtQkFBbUI7WUFDbkIsdUJBQXVCO1lBQ3ZCLG1CQUFtQjtZQUNuQixlQUFlO1lBQ2YsaUJBQWlCO1lBQ2pCLGNBQWM7WUFDZCxpQkFBaUIsRUFBQTs7QUF2SDdCO1VBNEhVLHNCQUFzQjtVQUN0QixjQUFjLEVBQUE7O0FBN0h4QjtZQWdJWSxzQkFBc0I7WUFDdEIsaUJBQWlCO1lBQ2pCLG1CQUFtQixFQUFBOztBQWxJL0I7WUFzSVksYUFBYTtZQUNiLGdCQUFnQjtZQUNoQixjQUFjO1lBQ2QsV0FBVyxFQUFBOztBQXpJdkI7Y0E0SWMsY0FBYyxFQUFBOztBQTVJNUI7Z0JBK0lnQixZQUFZLEVBQUE7O0FBL0k1QjtjQW9KYyxjQUFjO2NBQ2QsaUJBQWlCO2NBQ2pCLG1CQUFtQjtjQUNuQixvQkFBb0IsRUFBQTs7QUF2SmxDO1FBOEpRLGFBQWEsRUFBQTs7QUE5SnJCO0lBb0tJLFdBQVc7SUFDWCxrQkFBa0I7SUFDbEIsU0FBUztJQUNULFdBQVc7SUFDWCx3QkFBd0I7SUFDeEIsWUFBWSxFQUFBOztBQUloQjtFQUNFLGNBQWM7RUFDZCxvQkFBb0IsRUFBQTs7QUFGdEI7SUFLSSxlQUFlLEVBQUE7O0FBTG5CO01BUU0sYUFBYTtNQUNiLG1CQUFtQjtNQUNuQix1QkFBdUI7TUFDdkIsWUFBWTtNQUNaLGdCQUFnQjtNQUNoQixpQkFBaUI7TUFDakIsYUFBYTtNQUNiLGVBQWU7TUFDZixXQUFXLEVBQUE7O0FBaEJqQjtRQW1CUSwyQ0FBNEQsRUFBQTs7QUFuQnBFO1FBdUJRLG9CQUFvQjtRQUNwQixhQUFhO1FBQ2IsY0FBYyxFQUFBOztBQXpCdEI7VUE0QlUsa0RBQTJEO2tCQUEzRCwwQ0FBMkQsRUFBQTs7QUE1QnJFO1VBZ0NVLGtEQUEyRDtrQkFBM0QsMENBQTJELEVBQUE7O0FBaENyRTtVQW9DVSxnREFBeUQ7a0JBQXpELHdDQUF5RCxFQUFBOztBQU9uRTtFQUNFLGFBQWE7RUFDYixtQkFBbUI7RUFDbkIsMkJBQTJCO0VBQzNCLGNBQWM7RUFDZCxpQkFBaUIsRUFBQTs7QUFMbkI7SUFRSSxrQkFBa0I7SUFDbEIsWUFBWTtJQUNaLGdCQUFnQixFQUFBOztBQVZwQjtNQWFNLGtCQUFrQjtNQUNsQixjQUFjO01BQ2QsbUJBQW1CO01BQ25CLG9CQUFvQixFQUFBOztBQWhCMUI7UUFtQlEsV0FBVztRQUNYLGtCQUFrQjtRQUNsQixNQUFNO1FBQ04sT0FBTztRQUNQLGtCQUFrQjtRQUNsQixhQUFhO1FBQ2IsY0FBYyxFQUFBOztBQXpCdEI7TUE4Qk0saUJBQWlCLEVBQUE7O0FBOUJ2QjtNQWtDTSxrQkFBa0I7TUFDbEIsU0FBUztNQUNULE9BQU87TUFDUCxjQUFjO01BQ2QsV0FBVyxFQUFBOztBQXRDakI7UUF5Q1EsYUFBYSxFQUFBOztBQXpDckI7VUE0Q1UsY0FBYyxFQUFBOztBQTVDeEI7WUErQ1ksWUFBWSxFQUFBOztBQS9DeEI7VUFvRFUsY0FBYztVQUNkLGlCQUFpQjtVQUNqQixtQkFBbUI7VUFDbkIsb0JBQW9CLEVBQUE7O0FBdkQ5QjtRQTREUSwwQ0FBa0M7Z0JBQWxDLGtDQUFrQztRQUNsQywrbENBc0JHO1FBQ0gsMEJBQTBCO1FBQzFCLFlBQVksRUFBQTs7QUFyRnBCO0lBMkZJLGFBQWE7SUFDYixZQUFZO0lBQ1osaUJBQWlCO0lBQ2pCLGlCQUFpQixFQUFBOztBQTlGckI7TUFpR00sY0FBYztNQUNkLGlCQUFpQjtNQUNqQixtQkFBbUI7TUFDbkIsZ0JBQWdCLEVBQUE7O0FBcEd0QjtRQXVHUSxpQkFBaUIsRUFBQTs7QUF2R3pCO01BNEdNLGNBQWM7TUFDZCx5QkFBeUI7TUFDekIsYUFBYTtNQUNiLGNBQWMsRUFBQTs7QUEvR3BCO1FBa0hRLGdEQUF5RDtnQkFBekQsd0NBQXlELEVBQUE7O0FBbEhqRTtRQXNIUSw4Q0FBdUQ7Z0JBQXZELHNDQUF1RCxFQUFBOztBQU0vRDtFQUNFO0lBQ0UsK0JBQStCLEVBQUE7RUFFakM7SUFDRSw4QkFBOEIsRUFBQSxFQUFBOztBQUxsQztFQUNFO0lBQ0UsK0JBQStCLEVBQUE7RUFFakM7SUFDRSw4QkFBOEIsRUFBQSxFQUFBIiwiZmlsZSI6ImFwcC9zaWRlYmFyL3NpZGViYXIuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyI6aG9zdCB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICBmbGV4OiAwIDAgMjVyZW07XHJcbiAgcGFkZGluZzogMCAzcmVtO1xyXG4gIG1heC13aWR0aDogMjVyZW07XHJcbn1cclxuXHJcbi5zaWRlYmFyLWFjY291bnRzIHtcclxuICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gIGZsZXg6IDEgMSBhdXRvO1xyXG5cclxuICAuc2lkZWJhci1hY2NvdW50cy1oZWFkZXIge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgICBmbGV4OiAwIDAgYXV0bztcclxuICAgIGhlaWdodDogOHJlbTtcclxuICAgIGZvbnQtd2VpZ2h0OiA0MDA7XHJcblxyXG4gICAgaDMge1xyXG4gICAgICBmb250LXNpemU6IDEuN3JlbTtcclxuICAgIH1cclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBiYWNrZ3JvdW5kOiB0cmFuc3BhcmVudDtcclxuICAgICAgYm9yZGVyOiBub25lO1xyXG4gICAgICBvdXRsaW5lOiBub25lO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnNpZGViYXItYWNjb3VudHMtbGlzdCB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgIGZsZXg6IDEgMSBhdXRvO1xyXG4gICAgbWFyZ2luOiAwIC0zcmVtO1xyXG4gICAgb3ZlcmZsb3cteTogb3ZlcmxheTtcclxuXHJcbiAgICAuc2lkZWJhci1hY2NvdW50IHtcclxuICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgICAgZmxleC1zaHJpbms6IDA7XHJcbiAgICAgIGN1cnNvcjogcG9pbnRlcjtcclxuICAgICAgcGFkZGluZzogMnJlbSAzcmVtO1xyXG5cclxuICAgICAgLnNpZGViYXItYWNjb3VudC1yb3cge1xyXG4gICAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgICAgICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcblxyXG4gICAgICAgICYuYWNjb3VudC10aXRsZS1iYWxhbmNlIHtcclxuICAgICAgICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XHJcblxyXG4gICAgICAgICAgLnRpdGxlIHtcclxuICAgICAgICAgICAgZm9udC1zaXplOiAxLjVyZW07XHJcbiAgICAgICAgICAgIHRleHQtb3ZlcmZsb3c6IGVsbGlwc2lzO1xyXG4gICAgICAgICAgICBvdmVyZmxvdzogaGlkZGVuO1xyXG4gICAgICAgICAgICB3aGl0ZS1zcGFjZTogbm93cmFwO1xyXG4gICAgICAgICAgfVxyXG5cclxuICAgICAgICAgIC5iYWxhbmNlIHtcclxuICAgICAgICAgICAgZm9udC1zaXplOiAxLjhyZW07XHJcbiAgICAgICAgICAgIGZvbnQtd2VpZ2h0OiA2MDA7XHJcbiAgICAgICAgICAgIHdoaXRlLXNwYWNlOiBub3dyYXA7XHJcbiAgICAgICAgICB9XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAmLmFjY291bnQtYWxpYXMge1xyXG4gICAgICAgICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICAgICAgICBsaW5lLWhlaWdodDogMy40cmVtO1xyXG4gICAgICAgICAgbWFyZ2luLWJvdHRvbTogMC43cmVtO1xyXG5cclxuICAgICAgICAgIC5uYW1lIHtcclxuICAgICAgICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgICAgICAgICAgZmxleC1zaHJpbms6IDE7XHJcbiAgICAgICAgICAgIGxpbmUtaGVpZ2h0OiAxLjZyZW07XHJcbiAgICAgICAgICAgIHBhZGRpbmctcmlnaHQ6IDFyZW07XHJcbiAgICAgICAgICAgIG92ZXJmbG93OiBoaWRkZW47XHJcblxyXG4gICAgICAgICAgICBzcGFuIHtcclxuICAgICAgICAgICAgICB0ZXh0LW92ZXJmbG93OiBlbGxpcHNpcztcclxuICAgICAgICAgICAgICBvdmVyZmxvdzogaGlkZGVuO1xyXG4gICAgICAgICAgICAgIHdoaXRlLXNwYWNlOiBub3dyYXA7XHJcbiAgICAgICAgICAgIH1cclxuICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAucHJpY2Uge1xyXG4gICAgICAgICAgICBmbGV4LXNocmluazogMDtcclxuICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAuaWNvbiB7XHJcbiAgICAgICAgICAgIG1hcmdpbi1sZWZ0OiAwLjVyZW07XHJcbiAgICAgICAgICAgIHdpZHRoOiAxLjJyZW07XHJcbiAgICAgICAgICAgIGhlaWdodDogMS4ycmVtO1xyXG5cclxuICAgICAgICAgICAgJi5jb21tZW50IHtcclxuICAgICAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2FsZXJ0LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICAgICAgfVxyXG4gICAgICAgICAgfVxyXG4gICAgICAgIH1cclxuXHJcbiAgICAgICAgJi5hY2NvdW50LXN0YWtpbmcge1xyXG4gICAgICAgICAgbGluZS1oZWlnaHQ6IDIuOXJlbTtcclxuXHJcbiAgICAgICAgICAudGV4dCB7XHJcbiAgICAgICAgICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgICAgICAgfVxyXG4gICAgICAgIH1cclxuXHJcbiAgICAgICAgJi5hY2NvdW50LW1lc3NhZ2VzIHtcclxuICAgICAgICAgIGxpbmUtaGVpZ2h0OiAyLjdyZW07XHJcblxyXG4gICAgICAgICAgLnRleHQge1xyXG4gICAgICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcclxuICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAuaW5kaWNhdG9yIHtcclxuICAgICAgICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgICAgICAgICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XHJcbiAgICAgICAgICAgIGJvcmRlci1yYWRpdXM6IDFyZW07XHJcbiAgICAgICAgICAgIGZvbnQtc2l6ZTogMXJlbTtcclxuICAgICAgICAgICAgbWluLXdpZHRoOiAyLjRyZW07XHJcbiAgICAgICAgICAgIGhlaWdodDogMS42cmVtO1xyXG4gICAgICAgICAgICBwYWRkaW5nOiAwIDAuNXJlbTtcclxuICAgICAgICAgIH1cclxuICAgICAgICB9XHJcblxyXG4gICAgICAgICYuYWNjb3VudC1zeW5jaHJvbml6YXRpb24ge1xyXG4gICAgICAgICAgZmxleC1kaXJlY3Rpb246IGNvbHVtbjtcclxuICAgICAgICAgIGhlaWdodDogNS42cmVtO1xyXG5cclxuICAgICAgICAgIC5zdGF0dXMge1xyXG4gICAgICAgICAgICBhbGlnbi1zZWxmOiBmbGV4LXN0YXJ0O1xyXG4gICAgICAgICAgICBmb250LXNpemU6IDEuM3JlbTtcclxuICAgICAgICAgICAgbGluZS1oZWlnaHQ6IDIuNnJlbTtcclxuICAgICAgICAgIH1cclxuXHJcbiAgICAgICAgICAucHJvZ3Jlc3MtYmFyLWNvbnRhaW5lciB7XHJcbiAgICAgICAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgICAgICAgIG1hcmdpbjogMC40cmVtIDA7XHJcbiAgICAgICAgICAgIGhlaWdodDogMC43cmVtO1xyXG4gICAgICAgICAgICB3aWR0aDogMTAwJTtcclxuXHJcbiAgICAgICAgICAgIC5wcm9ncmVzcy1iYXIge1xyXG4gICAgICAgICAgICAgIGZsZXg6IDEgMCBhdXRvO1xyXG5cclxuICAgICAgICAgICAgICAuZmlsbCB7XHJcbiAgICAgICAgICAgICAgICBoZWlnaHQ6IDEwMCU7XHJcbiAgICAgICAgICAgICAgfVxyXG4gICAgICAgICAgICB9XHJcblxyXG4gICAgICAgICAgICAucHJvZ3Jlc3MtcGVyY2VudCB7XHJcbiAgICAgICAgICAgICAgZmxleDogMCAwIGF1dG87XHJcbiAgICAgICAgICAgICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICAgICAgICAgICAgbGluZS1oZWlnaHQ6IDAuN3JlbTtcclxuICAgICAgICAgICAgICBwYWRkaW5nLWxlZnQ6IDAuN3JlbTtcclxuICAgICAgICAgICAgfVxyXG4gICAgICAgICAgfVxyXG4gICAgICAgIH1cclxuICAgICAgfVxyXG5cclxuICAgICAgJjpmb2N1cyB7XHJcbiAgICAgICAgb3V0bGluZTogbm9uZTtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgJjphZnRlciB7XHJcbiAgICBjb250ZW50OiAnJztcclxuICAgIHBvc2l0aW9uOiBhYnNvbHV0ZTtcclxuICAgIGJvdHRvbTogMDtcclxuICAgIGxlZnQ6IC0zcmVtO1xyXG4gICAgd2lkdGg6IGNhbGMoMTAwJSArIDZyZW0pO1xyXG4gICAgaGVpZ2h0OiA1cmVtO1xyXG4gIH1cclxufVxyXG5cclxuLnNpZGViYXItc2V0dGluZ3Mge1xyXG4gIGZsZXg6IDAgMCBhdXRvO1xyXG4gIHBhZGRpbmctYm90dG9tOiAxcmVtO1xyXG5cclxuICAud3JhcC1idXR0b24ge1xyXG4gICAgbWFyZ2luOiAwIC0zcmVtO1xyXG5cclxuICAgIGJ1dHRvbiB7XHJcbiAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICAgIGJhY2tncm91bmQ6IHRyYW5zcGFyZW50O1xyXG4gICAgICBib3JkZXI6IG5vbmU7XHJcbiAgICAgIGZvbnQtd2VpZ2h0OiA0MDA7XHJcbiAgICAgIGxpbmUtaGVpZ2h0OiAzcmVtO1xyXG4gICAgICBvdXRsaW5lOiBub25lO1xyXG4gICAgICBwYWRkaW5nOiAwIDNyZW07XHJcbiAgICAgIHdpZHRoOiAxMDAlO1xyXG5cclxuICAgICAgJi5kaXNhYmxlZCB7XHJcbiAgICAgICAgY3Vyc29yOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL25vdC1hbGxvd2VkLnN2ZyksIG5vdC1hbGxvd2VkO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAuaWNvbiB7XHJcbiAgICAgICAgbWFyZ2luLXJpZ2h0OiAxLjJyZW07XHJcbiAgICAgICAgd2lkdGg6IDEuN3JlbTtcclxuICAgICAgICBoZWlnaHQ6IDEuN3JlbTtcclxuXHJcbiAgICAgICAgJi5jb250YWN0cyB7XHJcbiAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2NvbnRhY3RzLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICB9XHJcblxyXG4gICAgICAgICYuc2V0dGluZ3Mge1xyXG4gICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9zZXR0aW5ncy5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAmLmxvZ291dCB7XHJcbiAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2xvZ291dC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgICAgfVxyXG4gICAgICB9XHJcbiAgICB9XHJcbiAgfVxyXG59XHJcblxyXG4uc2lkZWJhci1zeW5jaHJvbml6YXRpb24tc3RhdHVzIHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAganVzdGlmeS1jb250ZW50OiBmbGV4LXN0YXJ0O1xyXG4gIGZsZXg6IDAgMCA3cmVtO1xyXG4gIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG5cclxuICAuc3RhdHVzLWNvbnRhaW5lciB7XHJcbiAgICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgICBmbGV4LWdyb3c6IDE7XHJcbiAgICB0ZXh0LWFsaWduOiBsZWZ0O1xyXG5cclxuICAgIC5vZmZsaW5lLCAub25saW5lIHtcclxuICAgICAgcG9zaXRpb246IHJlbGF0aXZlO1xyXG4gICAgICBkaXNwbGF5OiBibG9jaztcclxuICAgICAgbGluZS1oZWlnaHQ6IDEuMnJlbTtcclxuICAgICAgcGFkZGluZy1sZWZ0OiAyLjJyZW07XHJcblxyXG4gICAgICAmOmJlZm9yZSB7XHJcbiAgICAgICAgY29udGVudDogJyc7XHJcbiAgICAgICAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gICAgICAgIHRvcDogMDtcclxuICAgICAgICBsZWZ0OiAwO1xyXG4gICAgICAgIGJvcmRlci1yYWRpdXM6IDUwJTtcclxuICAgICAgICB3aWR0aDogMS4ycmVtO1xyXG4gICAgICAgIGhlaWdodDogMS4ycmVtO1xyXG4gICAgICB9XHJcbiAgICB9XHJcblxyXG4gICAgLnN5bmNpbmcsIC5sb2FkaW5nIHtcclxuICAgICAgbGluZS1oZWlnaHQ6IDRyZW07XHJcbiAgICB9XHJcblxyXG4gICAgLnByb2dyZXNzLWJhci1jb250YWluZXIge1xyXG4gICAgICBwb3NpdGlvbjogYWJzb2x1dGU7XHJcbiAgICAgIGJvdHRvbTogMDtcclxuICAgICAgbGVmdDogMDtcclxuICAgICAgaGVpZ2h0OiAwLjdyZW07XHJcbiAgICAgIHdpZHRoOiAxMDAlO1xyXG5cclxuICAgICAgLnN5bmNpbmcge1xyXG4gICAgICAgIGRpc3BsYXk6IGZsZXg7XHJcblxyXG4gICAgICAgIC5wcm9ncmVzcy1iYXIge1xyXG4gICAgICAgICAgZmxleDogMSAwIGF1dG87XHJcblxyXG4gICAgICAgICAgLmZpbGwge1xyXG4gICAgICAgICAgICBoZWlnaHQ6IDEwMCU7XHJcbiAgICAgICAgICB9XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAucHJvZ3Jlc3MtcGVyY2VudCB7XHJcbiAgICAgICAgICBmbGV4OiAwIDAgYXV0bztcclxuICAgICAgICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgICAgICAgbGluZS1oZWlnaHQ6IDAuN3JlbTtcclxuICAgICAgICAgIHBhZGRpbmctbGVmdDogMC43cmVtO1xyXG4gICAgICAgIH1cclxuICAgICAgfVxyXG5cclxuICAgICAgLmxvYWRpbmcge1xyXG4gICAgICAgIGFuaW1hdGlvbjogbW92ZSA1cyBsaW5lYXIgaW5maW5pdGU7XHJcbiAgICAgICAgYmFja2dyb3VuZC1pbWFnZTpcclxuICAgICAgICAgIC13ZWJraXQtZ3JhZGllbnQoXHJcbiAgICAgICAgICAgICAgbGluZWFyLCAwIDAsIDEwMCUgMTAwJSxcclxuICAgICAgICAgICAgICBjb2xvci1zdG9wKC4xMjUsIHJnYmEoMCwgMCwgMCwgLjE1KSksIGNvbG9yLXN0b3AoLjEyNSwgdHJhbnNwYXJlbnQpLFxyXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjI1MCwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKC4yNTAsIHJnYmEoMCwgMCwgMCwgLjEwKSksXHJcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguMzc1LCByZ2JhKDAsIDAsIDAsIC4xMCkpLCBjb2xvci1zdG9wKC4zNzUsIHRyYW5zcGFyZW50KSxcclxuICAgICAgICAgICAgICBjb2xvci1zdG9wKC41MDAsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCguNTAwLCByZ2JhKDAsIDAsIDAsIC4xNSkpLFxyXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjYyNSwgcmdiYSgwLCAwLCAwLCAuMTUpKSwgY29sb3Itc3RvcCguNjI1LCB0cmFuc3BhcmVudCksXHJcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguNzUwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjc1MCwgcmdiYSgwLCAwLCAwLCAuMTApKSxcclxuICAgICAgICAgICAgICBjb2xvci1zdG9wKC44NzUsIHJnYmEoMCwgMCwgMCwgLjEwKSksIGNvbG9yLXN0b3AoLjg3NSwgdHJhbnNwYXJlbnQpLFxyXG4gICAgICAgICAgICAgIHRvKHRyYW5zcGFyZW50KVxyXG4gICAgICAgICAgKSxcclxuICAgICAgICAgIC13ZWJraXQtZ3JhZGllbnQoXHJcbiAgICAgICAgICAgICAgbGluZWFyLCAwIDEwMCUsIDEwMCUgMCxcclxuICAgICAgICAgICAgICBjb2xvci1zdG9wKC4xMjUsIHJnYmEoMCwgMCwgMCwgLjMwKSksIGNvbG9yLXN0b3AoLjEyNSwgdHJhbnNwYXJlbnQpLFxyXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjI1MCwgdHJhbnNwYXJlbnQpLCBjb2xvci1zdG9wKC4yNTAsIHJnYmEoMCwgMCwgMCwgLjI1KSksXHJcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguMzc1LCByZ2JhKDAsIDAsIDAsIC4yNSkpLCBjb2xvci1zdG9wKC4zNzUsIHRyYW5zcGFyZW50KSxcclxuICAgICAgICAgICAgICBjb2xvci1zdG9wKC41MDAsIHRyYW5zcGFyZW50KSwgY29sb3Itc3RvcCguNTAwLCByZ2JhKDAsIDAsIDAsIC4zMCkpLFxyXG4gICAgICAgICAgICAgIGNvbG9yLXN0b3AoLjYyNSwgcmdiYSgwLCAwLCAwLCAuMzApKSwgY29sb3Itc3RvcCguNjI1LCB0cmFuc3BhcmVudCksXHJcbiAgICAgICAgICAgICAgY29sb3Itc3RvcCguNzUwLCB0cmFuc3BhcmVudCksIGNvbG9yLXN0b3AoLjc1MCwgcmdiYSgwLCAwLCAwLCAuMjUpKSxcclxuICAgICAgICAgICAgICBjb2xvci1zdG9wKC44NzUsIHJnYmEoMCwgMCwgMCwgLjI1KSksIGNvbG9yLXN0b3AoLjg3NSwgdHJhbnNwYXJlbnQpLFxyXG4gICAgICAgICAgICAgIHRvKHRyYW5zcGFyZW50KVxyXG4gICAgICAgICAgKTtcclxuICAgICAgICBiYWNrZ3JvdW5kLXNpemU6IDdyZW0gN3JlbTtcclxuICAgICAgICBoZWlnaHQ6IDEwMCU7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcblxyXG4gIC51cGRhdGUtY29udGFpbmVyIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmbGV4LWdyb3c6IDE7XHJcbiAgICBtYXJnaW4tbGVmdDogMXJlbTtcclxuICAgIHRleHQtYWxpZ246IHJpZ2h0O1xyXG5cclxuICAgIC51cGRhdGUtdGV4dCB7XHJcbiAgICAgIGZsZXg6IDEgMSBhdXRvO1xyXG4gICAgICBmb250LXNpemU6IDEuMnJlbTtcclxuICAgICAgbGluZS1oZWlnaHQ6IDEuOHJlbTtcclxuICAgICAgdGV4dC1hbGlnbjogbGVmdDtcclxuXHJcbiAgICAgICYudGltZSB7XHJcbiAgICAgICAgZm9udC1zaXplOiAxLjFyZW07XHJcbiAgICAgIH1cclxuICAgIH1cclxuXHJcbiAgICAuaWNvbiB7XHJcbiAgICAgIGZsZXg6IDEgMCBhdXRvO1xyXG4gICAgICBtYXJnaW46IDAuM3JlbSAwIDAgMC42cmVtO1xyXG4gICAgICB3aWR0aDogMS4ycmVtO1xyXG4gICAgICBoZWlnaHQ6IDEuMnJlbTtcclxuXHJcbiAgICAgICYudXBkYXRlIHtcclxuICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL3VwZGF0ZS5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYudGltZSB7XHJcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy90aW1lLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxufVxyXG5cclxuQGtleWZyYW1lcyBtb3ZlIHtcclxuICAwJSB7XHJcbiAgICBiYWNrZ3JvdW5kLXBvc2l0aW9uOiAxMDAlIC03cmVtO1xyXG4gIH1cclxuICAxMDAlIHtcclxuICAgIGJhY2tncm91bmQtcG9zaXRpb246IDEwMCUgN3JlbTtcclxuICB9XHJcbn1cclxuIl19 */"

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
/* harmony import */ var _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../_helpers/services/modal.service */ "./src/app/_helpers/services/modal.service.ts");
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
    function SidebarComponent(route, router, variablesService, backend, modal, ngZone) {
        this.route = route;
        this.router = router;
        this.variablesService = variablesService;
        this.backend = backend;
        this.modal = modal;
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
    SidebarComponent.prototype.contactsRoute = function () {
        if (this.variablesService.appPass) {
            this.router.navigate(['/contacts']);
        }
        else {
            this.modal.prepareModal('error', 'CONTACTS.FORM_ERRORS.SET_MASTER_PASSWORD');
        }
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
            _helpers_services_modal_service__WEBPACK_IMPORTED_MODULE_4__["ModalService"],
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

module.exports = "<div class=\"chart-header\">\r\n  <div class=\"general\">\r\n    <div>\r\n      <span class=\"label\">{{ 'STAKING.TITLE' | translate }}</span>\r\n      <span class=\"value\">\r\n        <app-staking-switch [wallet_id]=\"variablesService.currentWallet.wallet_id\" [(staking)]=\"variablesService.currentWallet.staking\"></app-staking-switch>\r\n      </span>\r\n    </div>\r\n    <div>\r\n      <span class=\"label\">{{ 'STAKING.TITLE_PENDING' | translate }}</span>\r\n      <span class=\"value\">{{pending.total | intToMoney}} {{variablesService.defaultCurrency}}</span>\r\n    </div>\r\n    <div>\r\n      <span class=\"label\">{{ 'STAKING.TITLE_TOTAL' | translate }}</span>\r\n      <span class=\"value\">{{total | intToMoney}} {{variablesService.defaultCurrency}}</span>\r\n    </div>\r\n  </div>\r\n  <div class=\"selected\" *ngIf=\"selectedDate && selectedDate.date\">\r\n    <span>{{selectedDate.date | date : 'MMM. EEEE, dd, yyyy'}}</span>\r\n    <span>{{selectedDate.amount}} {{variablesService.defaultCurrency}}</span>\r\n  </div>\r\n</div>\r\n\r\n<div class=\"chart\">\r\n  <div [chart]=\"chart\"></div>\r\n</div>\r\n\r\n<div class=\"chart-options\">\r\n  <div class=\"title\">\r\n    {{ 'STAKING.TITLE_PERIOD' | translate }}\r\n  </div>\r\n  <div class=\"options\">\r\n    <ng-container *ngFor=\"let period of periods\">\r\n      <button type=\"button\" [class.active]=\"period.active\" (click)=\"changePeriod(period)\">{{period.title}}</button>\r\n    </ng-container>\r\n  </div>\r\n\r\n  <div class=\"title\">\r\n    {{ 'STAKING.TITLE_GROUP' | translate }}\r\n  </div>\r\n  <div class=\"options\">\r\n    <ng-container *ngFor=\"let group of groups\">\r\n      <button type=\"button\" [class.active]=\"group.active\" (click)=\"changeGroup(group)\">{{group.title}}</button>\r\n    </ng-container>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/staking/staking.component.scss":
/*!************************************************!*\
  !*** ./src/app/staking/staking.component.scss ***!
  \************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: flex;\n  flex-direction: column;\n  width: 100%; }\n\n.chart-header {\n  display: flex;\n  flex: 0 0 auto; }\n\n.chart-header .general {\n    display: flex;\n    flex-direction: column;\n    align-items: flex-start;\n    justify-content: center;\n    flex-grow: 1;\n    font-size: 1.3rem;\n    margin: -0.5rem 0; }\n\n.chart-header .general > div {\n      display: flex;\n      align-items: center;\n      margin: 0.5rem 0;\n      height: 2rem; }\n\n.chart-header .general > div .label {\n        display: inline-block;\n        width: 9rem; }\n\n.chart-header .selected {\n    display: flex;\n    flex-direction: column;\n    align-items: flex-end;\n    justify-content: center;\n    flex-grow: 1;\n    font-size: 1.8rem; }\n\n.chart-header .selected span {\n      line-height: 2.9rem; }\n\n.chart {\n  position: relative;\n  display: flex;\n  align-items: center;\n  flex: 1 1 auto;\n  min-height: 40rem; }\n\n.chart > div {\n    position: absolute;\n    width: 100%;\n    height: 100%; }\n\n.chart-options {\n  display: flex;\n  align-items: center;\n  height: 2.4rem;\n  flex: 0 0 auto; }\n\n.chart-options .title {\n    font-size: 1.3rem;\n    padding: 0 1rem; }\n\n.chart-options .title:first-child {\n      padding-left: 0; }\n\n.chart-options .options {\n    display: flex;\n    justify-content: space-between;\n    flex-grow: 1;\n    height: 100%; }\n\n.chart-options .options button {\n      display: flex;\n      align-items: center;\n      justify-content: center;\n      flex: 1 1 auto;\n      cursor: pointer;\n      font-size: 1.3rem;\n      margin: 0 0.1rem;\n      padding: 0;\n      height: 100%; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC9zdGFraW5nL0Q6XFxQcm9qZWN0c19ub3dcXFpBTk9cXHphbm9cXHNyY1xcZ3VpXFxxdC1kYWVtb25cXGh0bWxfc291cmNlXFxzcmMvYXBwXFxzdGFraW5nXFxzdGFraW5nLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsYUFBYTtFQUNiLHNCQUFzQjtFQUN0QixXQUFXLEVBQUE7O0FBR2I7RUFDRSxhQUFhO0VBQ2IsY0FBYyxFQUFBOztBQUZoQjtJQUtJLGFBQWE7SUFDYixzQkFBc0I7SUFDdEIsdUJBQXVCO0lBQ3ZCLHVCQUF1QjtJQUN2QixZQUFZO0lBQ1osaUJBQWlCO0lBQ2pCLGlCQUFpQixFQUFBOztBQVhyQjtNQWNNLGFBQWE7TUFDYixtQkFBbUI7TUFDbkIsZ0JBQWdCO01BQ2hCLFlBQVksRUFBQTs7QUFqQmxCO1FBb0JRLHFCQUFxQjtRQUNyQixXQUFXLEVBQUE7O0FBckJuQjtJQTJCSSxhQUFhO0lBQ2Isc0JBQXNCO0lBQ3RCLHFCQUFxQjtJQUNyQix1QkFBdUI7SUFDdkIsWUFBWTtJQUNaLGlCQUFpQixFQUFBOztBQWhDckI7TUFtQ00sbUJBQW1CLEVBQUE7O0FBS3pCO0VBQ0Usa0JBQWtCO0VBQ2xCLGFBQWE7RUFDYixtQkFBbUI7RUFDbkIsY0FBYztFQUNkLGlCQUFpQixFQUFBOztBQUxuQjtJQVFJLGtCQUFrQjtJQUNsQixXQUFXO0lBQ1gsWUFBWSxFQUFBOztBQUloQjtFQUNFLGFBQWE7RUFDYixtQkFBbUI7RUFDbkIsY0FBYztFQUNkLGNBQWMsRUFBQTs7QUFKaEI7SUFPSSxpQkFBaUI7SUFDakIsZUFBZSxFQUFBOztBQVJuQjtNQVdNLGVBQWUsRUFBQTs7QUFYckI7SUFnQkksYUFBYTtJQUNiLDhCQUE4QjtJQUM5QixZQUFZO0lBQ1osWUFBWSxFQUFBOztBQW5CaEI7TUFzQk0sYUFBYTtNQUNiLG1CQUFtQjtNQUNuQix1QkFBdUI7TUFDdkIsY0FBYztNQUNkLGVBQWU7TUFDZixpQkFBaUI7TUFDakIsZ0JBQWdCO01BQ2hCLFVBQVU7TUFDVixZQUFZLEVBQUEiLCJmaWxlIjoiYXBwL3N0YWtpbmcvc3Rha2luZy5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgd2lkdGg6IDEwMCU7XHJcbn1cclxuXHJcbi5jaGFydC1oZWFkZXIge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgZmxleDogMCAwIGF1dG87XHJcblxyXG4gIC5nZW5lcmFsIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gICAgYWxpZ24taXRlbXM6IGZsZXgtc3RhcnQ7XHJcbiAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgIGZsZXgtZ3JvdzogMTtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbWFyZ2luOiAtMC41cmVtIDA7XHJcblxyXG4gICAgPiBkaXYge1xyXG4gICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICBtYXJnaW46IDAuNXJlbSAwO1xyXG4gICAgICBoZWlnaHQ6IDJyZW07XHJcblxyXG4gICAgICAubGFiZWwge1xyXG4gICAgICAgIGRpc3BsYXk6IGlubGluZS1ibG9jaztcclxuICAgICAgICB3aWR0aDogOXJlbTtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnNlbGVjdGVkIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gICAgYWxpZ24taXRlbXM6IGZsZXgtZW5kO1xyXG4gICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XHJcbiAgICBmbGV4LWdyb3c6IDE7XHJcbiAgICBmb250LXNpemU6IDEuOHJlbTtcclxuXHJcbiAgICBzcGFuIHtcclxuICAgICAgbGluZS1oZWlnaHQ6IDIuOXJlbTtcclxuICAgIH1cclxuICB9XHJcbn1cclxuXHJcbi5jaGFydCB7XHJcbiAgcG9zaXRpb246IHJlbGF0aXZlO1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICBmbGV4OiAxIDEgYXV0bztcclxuICBtaW4taGVpZ2h0OiA0MHJlbTtcclxuXHJcbiAgPiBkaXYge1xyXG4gICAgcG9zaXRpb246IGFic29sdXRlO1xyXG4gICAgd2lkdGg6IDEwMCU7XHJcbiAgICBoZWlnaHQ6IDEwMCU7XHJcbiAgfVxyXG59XHJcblxyXG4uY2hhcnQtb3B0aW9ucyB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gIGhlaWdodDogMi40cmVtO1xyXG4gIGZsZXg6IDAgMCBhdXRvO1xyXG5cclxuICAudGl0bGUge1xyXG4gICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICBwYWRkaW5nOiAwIDFyZW07XHJcblxyXG4gICAgJjpmaXJzdC1jaGlsZHtcclxuICAgICAgcGFkZGluZy1sZWZ0OiAwO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLm9wdGlvbnMge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICAgIGZsZXgtZ3JvdzogMTtcclxuICAgIGhlaWdodDogMTAwJTtcclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgICAgZmxleDogMSAxIGF1dG87XHJcbiAgICAgIGN1cnNvcjogcG9pbnRlcjtcclxuICAgICAgZm9udC1zaXplOiAxLjNyZW07XHJcbiAgICAgIG1hcmdpbjogMCAwLjFyZW07XHJcbiAgICAgIHBhZGRpbmc6IDA7XHJcbiAgICAgIGhlaWdodDogMTAwJTtcclxuICAgIH1cclxuICB9XHJcbn1cclxuIl19 */"

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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span [routerLink]=\"['/wallet/' + wallet.wallet_id + '/history']\">{{ wallet.name }}</span>\r\n      <span>{{ 'BREADCRUMBS.TRANSFER_ALIAS' | translate }}</span>\r\n    </div>\r\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <form class=\"form-transfer\">\r\n\r\n    <div class=\"input-block alias-name\">\r\n      <label for=\"alias-name\">\r\n        {{ 'TRANSFER_ALIAS.NAME.LABEL' | translate }}\r\n      </label>\r\n      <input type=\"text\" id=\"alias-name\" [value]=\"alias.name\" placeholder=\"{{ 'EDIT_ALIAS.NAME.PLACEHOLDER' | translate }}\" readonly>\r\n    </div>\r\n\r\n    <div class=\"input-block textarea\">\r\n      <label for=\"alias-comment\">\r\n        {{ 'TRANSFER_ALIAS.COMMENT.LABEL' | translate }}\r\n      </label>\r\n      <textarea id=\"alias-comment\" [value]=\"alias.comment\" placeholder=\"{{ 'EDIT_ALIAS.COMMENT.PLACEHOLDER' | translate }}\" readonly></textarea>\r\n    </div>\r\n\r\n    <div class=\"input-block alias-transfer-address\">\r\n      <label for=\"alias-transfer\">\r\n        {{ 'TRANSFER_ALIAS.ADDRESS.LABEL' | translate }}\r\n      </label>\r\n      <input type=\"text\" id=\"alias-transfer\" [(ngModel)]=\"transferAddress\" [ngModelOptions]=\"{standalone: true}\" (ngModelChange)=\"changeAddress()\" placeholder=\"{{ 'TRANSFER_ALIAS.ADDRESS.PLACEHOLDER' | translate }}\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"transferAddress.length > 0 && (transferAddressAlias || !transferAddressValid || (transferAddressValid && !permissionSend) || notEnoughMoney)\">\r\n        <div *ngIf=\"!transferAddressValid\">\r\n          {{ 'TRANSFER_ALIAS.FORM_ERRORS.WRONG_ADDRESS' | translate }}\r\n        </div>\r\n        <div *ngIf=\"transferAddressAlias || (transferAddressValid && !permissionSend)\">\r\n          {{ 'TRANSFER_ALIAS.FORM_ERRORS.ALIAS_EXISTS' | translate }}\r\n        </div>\r\n        <div *ngIf=\"notEnoughMoney\">\r\n          {{ 'TRANSFER_ALIAS.FORM_ERRORS.NO_MONEY' | translate }}\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"alias-cost\">{{ \"TRANSFER_ALIAS.COST\" | translate : {value: variablesService.default_fee, currency: variablesService.defaultCurrency} }}</div>\r\n\r\n    <div class=\"wrap-buttons\">\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"transferAlias()\" [disabled]=\"transferAddressAlias || !transferAddressValid || notEnoughMoney\">{{ 'TRANSFER_ALIAS.BUTTON_TRANSFER' | translate }}</button>\r\n    </div>\r\n\r\n  </form>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/transfer-alias/transfer-alias.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/transfer-alias/transfer-alias.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-transfer {\n  margin: 2.4rem 0; }\n  .form-transfer .alias-name {\n    width: 50%; }\n  .form-transfer .alias-cost {\n    font-size: 1.3rem;\n    margin-top: 2rem; }\n  .form-transfer .wrap-buttons {\n    display: flex;\n    justify-content: space-between;\n    margin: 2.5rem -0.7rem; }\n  .form-transfer .wrap-buttons button {\n      margin: 0 0.7rem;\n      width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC90cmFuc2Zlci1hbGlhcy9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcdHJhbnNmZXItYWxpYXNcXHRyYW5zZmVyLWFsaWFzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsZ0JBQWdCLEVBQUE7RUFEbEI7SUFJSSxVQUFVLEVBQUE7RUFKZDtJQVFJLGlCQUFpQjtJQUNqQixnQkFBZ0IsRUFBQTtFQVRwQjtJQWFJLGFBQWE7SUFDYiw4QkFBOEI7SUFDOUIsc0JBQXNCLEVBQUE7RUFmMUI7TUFrQk0sZ0JBQWdCO01BQ2hCLFlBQVksRUFBQSIsImZpbGUiOiJhcHAvdHJhbnNmZXItYWxpYXMvdHJhbnNmZXItYWxpYXMuY29tcG9uZW50LnNjc3MiLCJzb3VyY2VzQ29udGVudCI6WyIuZm9ybS10cmFuc2ZlciB7XHJcbiAgbWFyZ2luOiAyLjRyZW0gMDtcclxuXHJcbiAgLmFsaWFzLW5hbWUge1xyXG4gICAgd2lkdGg6IDUwJTtcclxuICB9XHJcblxyXG4gIC5hbGlhcy1jb3N0IHtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG4gICAgbWFyZ2luLXRvcDogMnJlbTtcclxuICB9XHJcblxyXG4gIC53cmFwLWJ1dHRvbnMge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICAgIG1hcmdpbjogMi41cmVtIC0wLjdyZW07XHJcblxyXG4gICAgYnV0dG9uIHtcclxuICAgICAgbWFyZ2luOiAwIDAuN3JlbTtcclxuICAgICAgd2lkdGg6IDE1cmVtO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG4iXX0= */"

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

module.exports = "<div class=\"head\">\r\n  <div class=\"interlocutor\">\r\n    @bitmain\r\n  </div>\r\n  <a class=\"back-btn\" [routerLink]=\"['/main']\">\r\n    <i class=\"icon back\"></i>\r\n    <span>{{ 'COMMON.BACK' | translate }}</span>\r\n  </a>\r\n</div>\r\n\r\n<div class=\"messages-content\">\r\n  <div class=\"messages-list scrolled-content\">\r\n    <div class=\"date\">10:39</div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"date\">11:44</div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"date\">12:15</div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"date\">13:13</div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n    <div class=\"buddy\">\r\n      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n    </div>\r\n    <div class=\"my\">\r\n      Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\r\n    </div>\r\n  </div>\r\n  <div class=\"type-message\">\r\n    <div class=\"input-block textarea\">\r\n      <textarea placeholder=\"{{ 'MESSAGES.SEND_PLACEHOLDER' | translate }}\"></textarea>\r\n    </div>\r\n    <button type=\"button\" class=\"blue-button\">{{ 'MESSAGES.SEND_BUTTON' | translate }}</button>\r\n  </div>\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/typing-message/typing-message.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/typing-message/typing-message.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  display: flex;\n  flex-direction: column;\n  width: 100%; }\n\n.head {\n  flex: 0 0 auto;\n  box-sizing: content-box;\n  margin: -3rem -3rem 0; }\n\n.messages-content {\n  display: flex;\n  flex-direction: column;\n  justify-content: space-between;\n  flex-grow: 1; }\n\n.messages-content .messages-list {\n    display: flex;\n    flex-direction: column;\n    font-size: 1.3rem;\n    margin: 1rem -3rem;\n    padding: 0 3rem;\n    overflow-y: overlay; }\n\n.messages-content .messages-list div {\n      margin: 0.7rem 0; }\n\n.messages-content .messages-list div.date {\n        text-align: center; }\n\n.messages-content .messages-list div.my, .messages-content .messages-list div.buddy {\n        position: relative;\n        padding: 1.8rem;\n        max-width: 60%; }\n\n.messages-content .messages-list div.buddy {\n        align-self: flex-end; }\n\n.messages-content .type-message {\n    display: flex;\n    flex: 0 0 auto;\n    width: 100%;\n    height: 4.2rem; }\n\n.messages-content .type-message .input-block {\n      width: 100%; }\n\n.messages-content .type-message .input-block > textarea {\n        min-height: 4.2rem; }\n\n.messages-content .type-message button {\n      flex: 0 0 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC90eXBpbmctbWVzc2FnZS9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcdHlwaW5nLW1lc3NhZ2VcXHR5cGluZy1tZXNzYWdlLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0UsYUFBYTtFQUNiLHNCQUFzQjtFQUN0QixXQUFXLEVBQUE7O0FBR2I7RUFDRSxjQUFjO0VBQ2QsdUJBQXVCO0VBQ3ZCLHFCQUFxQixFQUFBOztBQUd2QjtFQUNFLGFBQWE7RUFDYixzQkFBc0I7RUFDdEIsOEJBQThCO0VBQzlCLFlBQVksRUFBQTs7QUFKZDtJQU9JLGFBQWE7SUFDYixzQkFBc0I7SUFDdEIsaUJBQWlCO0lBQ2pCLGtCQUFrQjtJQUNsQixlQUFlO0lBQ2YsbUJBQW1CLEVBQUE7O0FBWnZCO01BZU0sZ0JBQWdCLEVBQUE7O0FBZnRCO1FBa0JRLGtCQUFrQixFQUFBOztBQWxCMUI7UUFzQlEsa0JBQWtCO1FBQ2xCLGVBQWU7UUFDZixjQUFjLEVBQUE7O0FBeEJ0QjtRQTRCUSxvQkFBb0IsRUFBQTs7QUE1QjVCO0lBa0NJLGFBQWE7SUFDYixjQUFjO0lBQ2QsV0FBVztJQUNYLGNBQWMsRUFBQTs7QUFyQ2xCO01Bd0NNLFdBQVcsRUFBQTs7QUF4Q2pCO1FBMkNRLGtCQUFrQixFQUFBOztBQTNDMUI7TUFnRE0sZUFBZSxFQUFBIiwiZmlsZSI6ImFwcC90eXBpbmctbWVzc2FnZS90eXBpbmctbWVzc2FnZS5jb21wb25lbnQuc2NzcyIsInNvdXJjZXNDb250ZW50IjpbIjpob3N0IHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgd2lkdGg6IDEwMCU7XHJcbn1cclxuXHJcbi5oZWFkIHtcclxuICBmbGV4OiAwIDAgYXV0bztcclxuICBib3gtc2l6aW5nOiBjb250ZW50LWJveDtcclxuICBtYXJnaW46IC0zcmVtIC0zcmVtIDA7XHJcbn1cclxuXHJcbi5tZXNzYWdlcy1jb250ZW50IHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAganVzdGlmeS1jb250ZW50OiBzcGFjZS1iZXR3ZWVuO1xyXG4gIGZsZXgtZ3JvdzogMTtcclxuXHJcbiAgLm1lc3NhZ2VzLWxpc3Qge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgICBmb250LXNpemU6IDEuM3JlbTtcclxuICAgIG1hcmdpbjogMXJlbSAtM3JlbTtcclxuICAgIHBhZGRpbmc6IDAgM3JlbTtcclxuICAgIG92ZXJmbG93LXk6IG92ZXJsYXk7XHJcblxyXG4gICAgZGl2IHtcclxuICAgICAgbWFyZ2luOiAwLjdyZW0gMDtcclxuXHJcbiAgICAgICYuZGF0ZSB7XHJcbiAgICAgICAgdGV4dC1hbGlnbjogY2VudGVyO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLm15LCAmLmJ1ZGR5IHtcclxuICAgICAgICBwb3NpdGlvbjogcmVsYXRpdmU7XHJcbiAgICAgICAgcGFkZGluZzogMS44cmVtO1xyXG4gICAgICAgIG1heC13aWR0aDogNjAlO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLmJ1ZGR5IHtcclxuICAgICAgICBhbGlnbi1zZWxmOiBmbGV4LWVuZDtcclxuICAgICAgfVxyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnR5cGUtbWVzc2FnZSB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgZmxleDogMCAwIGF1dG87XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIGhlaWdodDogNC4ycmVtO1xyXG5cclxuICAgIC5pbnB1dC1ibG9jayB7XHJcbiAgICAgIHdpZHRoOiAxMDAlO1xyXG5cclxuICAgICAgPiB0ZXh0YXJlYSB7XHJcbiAgICAgICAgbWluLWhlaWdodDogNC4ycmVtO1xyXG4gICAgICB9XHJcbiAgICB9XHJcblxyXG4gICAgYnV0dG9uIHtcclxuICAgICAgZmxleDogMCAwIDE1cmVtO1xyXG4gICAgfVxyXG4gIH1cclxufVxyXG5cclxuIl19 */"

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

module.exports = "<div class=\"content\">\r\n\r\n  <div class=\"head\">\r\n    <div class=\"breadcrumbs\">\r\n      <span (click)=\"back()\">{{variablesService.currentWallet.name}}</span>\r\n      <span>{{ 'BREADCRUMBS.WALLET_DETAILS' | translate }}</span>\r\n    </div>\r\n    <button type=\"button\" class=\"back-btn\" (click)=\"back()\">\r\n      <i class=\"icon back\"></i>\r\n      <span>{{ 'COMMON.BACK' | translate }}</span>\r\n    </button>\r\n  </div>\r\n\r\n  <form class=\"form-details\" [formGroup]=\"detailsForm\" (ngSubmit)=\"onSubmitEdit()\">\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-name\">{{ 'WALLET_DETAILS.LABEL_NAME' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-name\" formControlName=\"name\" [maxLength]=\"variablesService.maxWalletNameLength\" (contextmenu)=\"variablesService.onContextMenu($event)\">\r\n      <div class=\"error-block\" *ngIf=\"detailsForm.controls['name'].invalid && (detailsForm.controls['name'].dirty || detailsForm.controls['name'].touched)\">\r\n        <div *ngIf=\"detailsForm.controls['name'].errors['required']\">\r\n          {{ 'WALLET_DETAILS.FORM_ERRORS.NAME_REQUIRED' | translate }}\r\n        </div>\r\n        <div *ngIf=\"detailsForm.controls['name'].errors['duplicate']\">\r\n          {{ 'WALLET_DETAILS.FORM_ERRORS.NAME_DUPLICATE' | translate }}\r\n        </div>\r\n      </div>\r\n      <div class=\"error-block\" *ngIf=\"detailsForm.get('name').value.length >= variablesService.maxWalletNameLength\">\r\n        {{ 'WALLET_DETAILS.FORM_ERRORS.MAX_LENGTH' | translate }}\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"input-block\">\r\n      <label for=\"wallet-location\">{{ 'WALLET_DETAILS.LABEL_FILE_LOCATION' | translate }}</label>\r\n      <input type=\"text\" id=\"wallet-location\" formControlName=\"path\" readonly>\r\n    </div>\r\n\r\n    <div class=\"input-block textarea\">\r\n      <label for=\"seed-phrase\">{{ 'WALLET_DETAILS.LABEL_SEED_PHRASE' | translate }}</label>\r\n      <div class=\"seed-phrase\" id=\"seed-phrase\">\r\n        <div class=\"seed-phrase-hint\" (click)=\"showSeedPhrase()\" *ngIf=\"!showSeed\">{{ 'WALLET_DETAILS.SEED_PHRASE_HINT' | translate }}</div>\r\n        <div class=\"seed-phrase-content\" *ngIf=\"showSeed\" (contextmenu)=\"variablesService.onContextMenuOnlyCopy($event, seedPhrase)\">\r\n          <ng-container *ngFor=\"let word of seedPhrase.split(' '); let index = index\">\r\n            <div class=\"word\">{{(index + 1) + '. ' + word}}</div>\r\n          </ng-container>\r\n        </div>\r\n      </div>\r\n    </div>\r\n\r\n    <div class=\"wallet-buttons\">\r\n      <button type=\"submit\" class=\"blue-button\" [disabled]=\"!detailsForm.valid\">{{ 'WALLET_DETAILS.BUTTON_SAVE' | translate }}</button>\r\n      <button type=\"button\" class=\"blue-button\" (click)=\"closeWallet()\">{{ 'WALLET_DETAILS.BUTTON_REMOVE' | translate }}</button>\r\n    </div>\r\n\r\n  </form>\r\n\r\n</div>\r\n"

/***/ }),

/***/ "./src/app/wallet-details/wallet-details.component.scss":
/*!**************************************************************!*\
  !*** ./src/app/wallet-details/wallet-details.component.scss ***!
  \**************************************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ".form-details {\n  margin-top: 1.8rem; }\n  .form-details .input-block:first-child {\n    width: 50%; }\n  .form-details .seed-phrase {\n    display: flex;\n    font-size: 1.4rem;\n    line-height: 1.5rem;\n    padding: 1.4rem;\n    width: 100%;\n    height: 8.8rem; }\n  .form-details .seed-phrase .seed-phrase-hint {\n      display: flex;\n      align-items: center;\n      justify-content: center;\n      cursor: pointer;\n      width: 100%;\n      height: 100%; }\n  .form-details .seed-phrase .seed-phrase-content {\n      display: flex;\n      flex-direction: column;\n      flex-wrap: wrap;\n      width: 100%;\n      height: 100%; }\n  .form-details .wallet-buttons {\n    display: flex;\n    align-items: center;\n    justify-content: space-between; }\n  .form-details .wallet-buttons button {\n      margin: 2.9rem 0;\n      width: 100%;\n      max-width: 15rem; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC93YWxsZXQtZGV0YWlscy9EOlxcUHJvamVjdHNfbm93XFxaQU5PXFx6YW5vXFxzcmNcXGd1aVxccXQtZGFlbW9uXFxodG1sX3NvdXJjZVxcc3JjL2FwcFxcd2FsbGV0LWRldGFpbHNcXHdhbGxldC1kZXRhaWxzLmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCLEVBQUE7RUFEcEI7SUFNTSxVQUFVLEVBQUE7RUFOaEI7SUFXSSxhQUFhO0lBQ2IsaUJBQWlCO0lBQ2pCLG1CQUFtQjtJQUNuQixlQUFlO0lBQ2YsV0FBVztJQUNYLGNBQWMsRUFBQTtFQWhCbEI7TUFtQk0sYUFBYTtNQUNiLG1CQUFtQjtNQUNuQix1QkFBdUI7TUFDdkIsZUFBZTtNQUNmLFdBQVc7TUFDWCxZQUFZLEVBQUE7RUF4QmxCO01BNEJNLGFBQWE7TUFDYixzQkFBc0I7TUFDdEIsZUFBZTtNQUNmLFdBQVc7TUFDWCxZQUFZLEVBQUE7RUFoQ2xCO0lBcUNJLGFBQWE7SUFDYixtQkFBbUI7SUFDbkIsOEJBQThCLEVBQUE7RUF2Q2xDO01BMENNLGdCQUFnQjtNQUNoQixXQUFXO01BQ1gsZ0JBQWdCLEVBQUEiLCJmaWxlIjoiYXBwL3dhbGxldC1kZXRhaWxzL3dhbGxldC1kZXRhaWxzLmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiLmZvcm0tZGV0YWlscyB7XHJcbiAgbWFyZ2luLXRvcDogMS44cmVtO1xyXG5cclxuICAuaW5wdXQtYmxvY2sge1xyXG5cclxuICAgICY6Zmlyc3QtY2hpbGQge1xyXG4gICAgICB3aWR0aDogNTAlO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgLnNlZWQtcGhyYXNlIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBmb250LXNpemU6IDEuNHJlbTtcclxuICAgIGxpbmUtaGVpZ2h0OiAxLjVyZW07XHJcbiAgICBwYWRkaW5nOiAxLjRyZW07XHJcbiAgICB3aWR0aDogMTAwJTtcclxuICAgIGhlaWdodDogOC44cmVtO1xyXG5cclxuICAgIC5zZWVkLXBocmFzZS1oaW50IHtcclxuICAgICAgZGlzcGxheTogZmxleDtcclxuICAgICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XHJcbiAgICAgIGN1cnNvcjogcG9pbnRlcjtcclxuICAgICAgd2lkdGg6IDEwMCU7XHJcbiAgICAgIGhlaWdodDogMTAwJTtcclxuICAgIH1cclxuXHJcbiAgICAuc2VlZC1waHJhc2UtY29udGVudCB7XHJcbiAgICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICAgIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgICAgIGZsZXgtd3JhcDogd3JhcDtcclxuICAgICAgd2lkdGg6IDEwMCU7XHJcbiAgICAgIGhlaWdodDogMTAwJTtcclxuICAgIH1cclxuICB9XHJcblxyXG4gIC53YWxsZXQtYnV0dG9ucyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuXHJcbiAgICBidXR0b24ge1xyXG4gICAgICBtYXJnaW46IDIuOXJlbSAwO1xyXG4gICAgICB3aWR0aDogMTAwJTtcclxuICAgICAgbWF4LXdpZHRoOiAxNXJlbTtcclxuICAgIH1cclxuICB9XHJcblxyXG59XHJcbiJdfQ== */"

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

module.exports = "<div class=\"header\">\r\n  <div>\r\n    <h3 tooltip=\"{{ variablesService.currentWallet.name }}\" placement=\"bottom-left\" tooltipClass=\"table-tooltip\" [delay]=\"500\" [showWhenNoOverflow]=\"false\">{{variablesService.currentWallet.name}}</h3>\r\n    <button [routerLink]=\"['/assign-alias']\" *ngIf=\"!variablesService.currentWallet.alias.hasOwnProperty('name') && variablesService.currentWallet.loaded && variablesService.daemon_state === 2 && variablesService.currentWallet.alias_available\">\r\n      <i class=\"icon account\"></i>\r\n      <span>{{ 'WALLET.REGISTER_ALIAS' | translate }}</span>\r\n    </button>\r\n    <div class=\"alias\" *ngIf=\"variablesService.currentWallet.alias.hasOwnProperty('name') && variablesService.currentWallet.loaded && variablesService.daemon_state === 2\">\r\n      <span>{{variablesService.currentWallet.alias['name']}}</span>\r\n      <ng-container *ngIf=\"variablesService.currentWallet.alias_available\">\r\n        <i class=\"icon edit\" [routerLink]=\"['/edit-alias']\"></i>\r\n        <i class=\"icon transfer\" [routerLink]=\"['/transfer-alias']\"></i>\r\n      </ng-container>\r\n    </div>\r\n  </div>\r\n  <div>\r\n    <button [routerLink]=\"['/details']\" routerLinkActive=\"active\">\r\n      <i class=\"icon details\"></i>\r\n    </button>\r\n    <button type=\"button\" (click)=\"closeWallet()\">\r\n      <i class=\"icon close-wallet\"></i>\r\n    </button>\r\n  </div>\r\n</div>\r\n<div class=\"address\">\r\n  <span>{{variablesService.currentWallet.address}}</span>\r\n  <i class=\"icon\" [class.copy]=\"!copyAnimation\" [class.copied]=\"copyAnimation\" (click)=\"copyAddress()\"></i>\r\n</div>\r\n<div class=\"balance\">\r\n  <span [tooltip]=\"getTooltip()\" [placement]=\"'bottom'\" [tooltipClass]=\"'balance-tooltip'\" [delay]=\"150\" [timeout]=\"0\" (onHide)=\"onHideTooltip()\">{{variablesService.currentWallet.balance | intToMoney  : '3'}} {{variablesService.defaultCurrency}}</span>\r\n  <span>$ {{variablesService.currentWallet.getMoneyEquivalent(variablesService.moneyEquivalent) | intToMoney | number : '1.2-2'}}</span>\r\n</div>\r\n<div class=\"tabs\">\r\n  <div class=\"tabs-header\">\r\n    <ng-container *ngFor=\"let tab of tabs; let index = index\">\r\n      <div class=\"tab\" [class.active]=\"tab.active\" [class.disabled]=\"(tab.link === '/send' || tab.link === '/contracts' || tab.link === '/staking') && (variablesService.daemon_state !== 2 || !variablesService.currentWallet.loaded)\" (click)=\"changeTab(index)\">\r\n        <i class=\"icon\" [ngClass]=\"tab.icon\"></i>\r\n        <span>{{ tab.title | translate }}</span>\r\n        <span class=\"indicator\" *ngIf=\"tab.indicator\">{{variablesService.currentWallet.new_contracts}}</span>\r\n      </div>\r\n    </ng-container>\r\n  </div>\r\n  <div #scrolledContent class=\"tabs-content scrolled-content\">\r\n    <router-outlet></router-outlet>\r\n  </div>\r\n</div>\r\n\r\n"

/***/ }),

/***/ "./src/app/wallet/wallet.component.scss":
/*!**********************************************!*\
  !*** ./src/app/wallet/wallet.component.scss ***!
  \**********************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = ":host {\n  position: relative;\n  display: flex;\n  flex-direction: column;\n  padding: 0 3rem 3rem;\n  min-width: 95rem;\n  width: 100%;\n  height: 100%; }\n\n.header {\n  display: flex;\n  align-items: center;\n  justify-content: space-between;\n  flex: 0 0 auto;\n  height: 8rem; }\n\n.header > div {\n    display: flex;\n    align-items: center; }\n\n.header > div :not(:last-child) {\n      margin-right: 3.2rem; }\n\n.header h3 {\n    font-size: 1.7rem;\n    font-weight: 600;\n    text-overflow: ellipsis;\n    overflow: hidden;\n    white-space: nowrap;\n    max-width: 50rem;\n    line-height: 2.7rem; }\n\n.header button {\n    display: flex;\n    align-items: center;\n    background: transparent;\n    border: none;\n    cursor: pointer;\n    font-weight: 400;\n    outline: none;\n    padding: 0; }\n\n.header button .icon {\n      margin-right: 1.2rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.header button .icon.account {\n        -webkit-mask: url('account.svg') no-repeat center;\n                mask: url('account.svg') no-repeat center; }\n\n.header button .icon.lock {\n        -webkit-mask: url('lock.svg') no-repeat center;\n                mask: url('lock.svg') no-repeat center; }\n\n.header button .icon.details {\n        -webkit-mask: url('details-settings.svg') no-repeat center;\n                mask: url('details-settings.svg') no-repeat center;\n        margin-right: 0; }\n\n.header button .icon.close-wallet {\n        -webkit-mask: url('close-wallet.svg') no-repeat center;\n                mask: url('close-wallet.svg') no-repeat center;\n        margin-right: 0; }\n\n.header .alias {\n    display: flex;\n    align-items: center;\n    font-size: 1.3rem; }\n\n.header .alias .icon {\n      cursor: pointer;\n      margin-right: 1.2rem;\n      width: 1.7rem;\n      height: 1.7rem; }\n\n.header .alias .icon.edit {\n        -webkit-mask: url('details.svg') no-repeat center;\n                mask: url('details.svg') no-repeat center; }\n\n.header .alias .icon.transfer {\n        -webkit-mask: url('send.svg') no-repeat center;\n                mask: url('send.svg') no-repeat center; }\n\n.address {\n  display: flex;\n  align-items: center;\n  flex: 0 0 auto;\n  font-size: 1.4rem;\n  line-height: 1.7rem; }\n\n.address .icon {\n    cursor: pointer;\n    margin-left: 1.2rem;\n    width: 1.7rem;\n    height: 1.7rem; }\n\n.address .icon.copy {\n      -webkit-mask: url('copy.svg') no-repeat center;\n              mask: url('copy.svg') no-repeat center; }\n\n.address .icon.copy:hover {\n        opacity: 0.75; }\n\n.address .icon.copied {\n      -webkit-mask: url('complete-testwallet.svg') no-repeat center;\n              mask: url('complete-testwallet.svg') no-repeat center; }\n\n.balance {\n  display: flex;\n  align-items: flex-end;\n  justify-content: flex-start;\n  flex: 0 0 auto;\n  margin: 2.6rem 0; }\n\n.balance :first-child {\n    font-size: 3.3rem;\n    font-weight: 600;\n    line-height: 2.4rem;\n    margin-right: 3.5rem; }\n\n.balance :last-child {\n    font-size: 1.8rem;\n    font-weight: 600;\n    line-height: 1.3rem; }\n\n.tabs {\n  display: flex;\n  flex-direction: column;\n  flex: 1 1 auto; }\n\n.tabs .tabs-header {\n    display: flex;\n    justify-content: space-between;\n    flex: 0 0 auto; }\n\n.tabs .tabs-header .tab {\n      display: flex;\n      align-items: center;\n      justify-content: center;\n      flex: 1 0 auto;\n      cursor: pointer;\n      padding: 0 1rem;\n      height: 5rem; }\n\n.tabs .tabs-header .tab .icon {\n        margin-right: 1.3rem;\n        width: 1.7rem;\n        height: 1.7rem; }\n\n.tabs .tabs-header .tab .icon.send {\n          -webkit-mask: url('send.svg') no-repeat center;\n                  mask: url('send.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.receive {\n          -webkit-mask: url('receive.svg') no-repeat center;\n                  mask: url('receive.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.history {\n          -webkit-mask: url('history.svg') no-repeat center;\n                  mask: url('history.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.contracts {\n          -webkit-mask: url('contracts.svg') no-repeat center;\n                  mask: url('contracts.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.messages {\n          -webkit-mask: url('message.svg') no-repeat center;\n                  mask: url('message.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .icon.staking {\n          -webkit-mask: url('staking.svg') no-repeat center;\n                  mask: url('staking.svg') no-repeat center; }\n\n.tabs .tabs-header .tab .indicator {\n        display: flex;\n        align-items: center;\n        justify-content: center;\n        border-radius: 1rem;\n        font-size: 1rem;\n        font-weight: 600;\n        margin-left: 1.3rem;\n        padding: 0 0.5rem;\n        min-width: 1.6rem;\n        height: 1.6rem; }\n\n.tabs .tabs-header .tab.disabled {\n        cursor: url('not-allowed.svg'), not-allowed; }\n\n.tabs .tabs-header .tab:not(:last-child) {\n        margin-right: 0.3rem; }\n\n.tabs .tabs-content {\n    display: flex;\n    padding: 3rem;\n    flex: 1 1 auto;\n    overflow-x: hidden;\n    overflow-y: overlay; }\n\n/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFwcC93YWxsZXQvRDpcXFByb2plY3RzX25vd1xcWkFOT1xcemFub1xcc3JjXFxndWlcXHF0LWRhZW1vblxcaHRtbF9zb3VyY2VcXHNyYy9hcHBcXHdhbGxldFxcd2FsbGV0LmNvbXBvbmVudC5zY3NzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBO0VBQ0Usa0JBQWtCO0VBQ2xCLGFBQWE7RUFDYixzQkFBc0I7RUFDdEIsb0JBQW9CO0VBQ3BCLGdCQUFnQjtFQUNoQixXQUFXO0VBQ1gsWUFBWSxFQUFBOztBQUdkO0VBQ0UsYUFBYTtFQUNiLG1CQUFtQjtFQUNuQiw4QkFBOEI7RUFDOUIsY0FBYztFQUNkLFlBQVksRUFBQTs7QUFMZDtJQVFJLGFBQWE7SUFDYixtQkFBbUIsRUFBQTs7QUFUdkI7TUFZTSxvQkFBb0IsRUFBQTs7QUFaMUI7SUFpQkksaUJBQWlCO0lBQ2pCLGdCQUFnQjtJQUNoQix1QkFBdUI7SUFDdkIsZ0JBQWdCO0lBQ2hCLG1CQUFtQjtJQUNuQixnQkFBZ0I7SUFDaEIsbUJBQW1CLEVBQUE7O0FBdkJ2QjtJQTJCSSxhQUFhO0lBQ2IsbUJBQW1CO0lBQ25CLHVCQUF1QjtJQUN2QixZQUFZO0lBQ1osZUFBZTtJQUNmLGdCQUFnQjtJQUNoQixhQUFhO0lBQ2IsVUFBVSxFQUFBOztBQWxDZDtNQXFDTSxvQkFBb0I7TUFDcEIsYUFBYTtNQUNiLGNBQWMsRUFBQTs7QUF2Q3BCO1FBMENRLGlEQUEwRDtnQkFBMUQseUNBQTBELEVBQUE7O0FBMUNsRTtRQThDUSw4Q0FBdUQ7Z0JBQXZELHNDQUF1RCxFQUFBOztBQTlDL0Q7UUFrRFEsMERBQW1FO2dCQUFuRSxrREFBbUU7UUFDbkUsZUFBZSxFQUFBOztBQW5EdkI7UUF1RFEsc0RBQStEO2dCQUEvRCw4Q0FBK0Q7UUFDL0QsZUFBZSxFQUFBOztBQXhEdkI7SUE4REksYUFBYTtJQUNiLG1CQUFtQjtJQUNuQixpQkFBaUIsRUFBQTs7QUFoRXJCO01BbUVNLGVBQWU7TUFDZixvQkFBb0I7TUFDcEIsYUFBYTtNQUNiLGNBQWMsRUFBQTs7QUF0RXBCO1FBeUVRLGlEQUEwRDtnQkFBMUQseUNBQTBELEVBQUE7O0FBekVsRTtRQTZFUSw4Q0FBdUQ7Z0JBQXZELHNDQUF1RCxFQUFBOztBQU0vRDtFQUNFLGFBQWE7RUFDYixtQkFBbUI7RUFDbkIsY0FBYztFQUNkLGlCQUFpQjtFQUNqQixtQkFBbUIsRUFBQTs7QUFMckI7SUFRSSxlQUFlO0lBQ2YsbUJBQW1CO0lBQ25CLGFBQWE7SUFDYixjQUFjLEVBQUE7O0FBWGxCO01BY00sOENBQXVEO2NBQXZELHNDQUF1RCxFQUFBOztBQWQ3RDtRQWlCUSxhQUFhLEVBQUE7O0FBakJyQjtNQXNCTSw2REFBc0U7Y0FBdEUscURBQXNFLEVBQUE7O0FBSzVFO0VBQ0UsYUFBYTtFQUNiLHFCQUFxQjtFQUNyQiwyQkFBMkI7RUFDM0IsY0FBYztFQUNkLGdCQUFnQixFQUFBOztBQUxsQjtJQVFJLGlCQUFpQjtJQUNqQixnQkFBZ0I7SUFDaEIsbUJBQW1CO0lBQ25CLG9CQUFvQixFQUFBOztBQVh4QjtJQWVJLGlCQUFpQjtJQUNqQixnQkFBZ0I7SUFDaEIsbUJBQW1CLEVBQUE7O0FBSXZCO0VBQ0UsYUFBYTtFQUNiLHNCQUFzQjtFQUN0QixjQUFjLEVBQUE7O0FBSGhCO0lBTUksYUFBYTtJQUNiLDhCQUE4QjtJQUM5QixjQUFjLEVBQUE7O0FBUmxCO01BV00sYUFBYTtNQUNiLG1CQUFtQjtNQUNuQix1QkFBdUI7TUFDdkIsY0FBYztNQUNkLGVBQWU7TUFDZixlQUFlO01BQ2YsWUFBWSxFQUFBOztBQWpCbEI7UUFvQlEsb0JBQW9CO1FBQ3BCLGFBQWE7UUFDYixjQUFjLEVBQUE7O0FBdEJ0QjtVQXlCVSw4Q0FBdUQ7a0JBQXZELHNDQUF1RCxFQUFBOztBQXpCakU7VUE2QlUsaURBQTBEO2tCQUExRCx5Q0FBMEQsRUFBQTs7QUE3QnBFO1VBaUNVLGlEQUEwRDtrQkFBMUQseUNBQTBELEVBQUE7O0FBakNwRTtVQXFDVSxtREFBNEQ7a0JBQTVELDJDQUE0RCxFQUFBOztBQXJDdEU7VUF5Q1UsaURBQTBEO2tCQUExRCx5Q0FBMEQsRUFBQTs7QUF6Q3BFO1VBNkNVLGlEQUEwRDtrQkFBMUQseUNBQTBELEVBQUE7O0FBN0NwRTtRQWtEUSxhQUFhO1FBQ2IsbUJBQW1CO1FBQ25CLHVCQUF1QjtRQUN2QixtQkFBbUI7UUFDbkIsZUFBZTtRQUNmLGdCQUFnQjtRQUNoQixtQkFBbUI7UUFDbkIsaUJBQWlCO1FBQ2pCLGlCQUFpQjtRQUNqQixjQUFjLEVBQUE7O0FBM0R0QjtRQStEUSwyQ0FBNEQsRUFBQTs7QUEvRHBFO1FBbUVRLG9CQUFvQixFQUFBOztBQW5FNUI7SUF5RUksYUFBYTtJQUNiLGFBQWE7SUFDYixjQUFjO0lBQ2Qsa0JBQWtCO0lBQ2xCLG1CQUFtQixFQUFBIiwiZmlsZSI6ImFwcC93YWxsZXQvd2FsbGV0LmNvbXBvbmVudC5zY3NzIiwic291cmNlc0NvbnRlbnQiOlsiOmhvc3Qge1xyXG4gIHBvc2l0aW9uOiByZWxhdGl2ZTtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGZsZXgtZGlyZWN0aW9uOiBjb2x1bW47XHJcbiAgcGFkZGluZzogMCAzcmVtIDNyZW07XHJcbiAgbWluLXdpZHRoOiA5NXJlbTtcclxuICB3aWR0aDogMTAwJTtcclxuICBoZWlnaHQ6IDEwMCU7XHJcbn1cclxuXHJcbi5oZWFkZXIge1xyXG4gIGRpc3BsYXk6IGZsZXg7XHJcbiAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IHNwYWNlLWJldHdlZW47XHJcbiAgZmxleDogMCAwIGF1dG87XHJcbiAgaGVpZ2h0OiA4cmVtO1xyXG5cclxuICA+IGRpdiB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuXHJcbiAgICA6bm90KDpsYXN0LWNoaWxkKSB7XHJcbiAgICAgIG1hcmdpbi1yaWdodDogMy4ycmVtO1xyXG4gICAgfVxyXG4gIH1cclxuXHJcbiAgaDMge1xyXG4gICAgZm9udC1zaXplOiAxLjdyZW07XHJcbiAgICBmb250LXdlaWdodDogNjAwO1xyXG4gICAgdGV4dC1vdmVyZmxvdzogZWxsaXBzaXM7XHJcbiAgICBvdmVyZmxvdzogaGlkZGVuO1xyXG4gICAgd2hpdGUtc3BhY2U6IG5vd3JhcDtcclxuICAgIG1heC13aWR0aDogNTByZW07XHJcbiAgICBsaW5lLWhlaWdodDogMi43cmVtO1xyXG4gIH1cclxuXHJcbiAgYnV0dG9uIHtcclxuICAgIGRpc3BsYXk6IGZsZXg7XHJcbiAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgYmFja2dyb3VuZDogdHJhbnNwYXJlbnQ7XHJcbiAgICBib3JkZXI6IG5vbmU7XHJcbiAgICBjdXJzb3I6IHBvaW50ZXI7XHJcbiAgICBmb250LXdlaWdodDogNDAwO1xyXG4gICAgb3V0bGluZTogbm9uZTtcclxuICAgIHBhZGRpbmc6IDA7XHJcblxyXG4gICAgLmljb24ge1xyXG4gICAgICBtYXJnaW4tcmlnaHQ6IDEuMnJlbTtcclxuICAgICAgd2lkdGg6IDEuN3JlbTtcclxuICAgICAgaGVpZ2h0OiAxLjdyZW07XHJcblxyXG4gICAgICAmLmFjY291bnQge1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvYWNjb3VudC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYubG9jayB7XHJcbiAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9sb2NrLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi5kZXRhaWxzIHtcclxuICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2RldGFpbHMtc2V0dGluZ3Muc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgIG1hcmdpbi1yaWdodDogMDtcclxuICAgICAgfVxyXG5cclxuICAgICAgJi5jbG9zZS13YWxsZXQge1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvY2xvc2Utd2FsbGV0LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICBtYXJnaW4tcmlnaHQ6IDA7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcblxyXG4gIC5hbGlhcyB7XHJcbiAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgYWxpZ24taXRlbXM6IGNlbnRlcjtcclxuICAgIGZvbnQtc2l6ZTogMS4zcmVtO1xyXG5cclxuICAgIC5pY29uIHtcclxuICAgICAgY3Vyc29yOiBwb2ludGVyO1xyXG4gICAgICBtYXJnaW4tcmlnaHQ6IDEuMnJlbTtcclxuICAgICAgd2lkdGg6IDEuN3JlbTtcclxuICAgICAgaGVpZ2h0OiAxLjdyZW07XHJcblxyXG4gICAgICAmLmVkaXQge1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvZGV0YWlscy5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICYudHJhbnNmZXIge1xyXG4gICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvc2VuZC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcbn1cclxuXHJcbi5hZGRyZXNzIHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgZmxleDogMCAwIGF1dG87XHJcbiAgZm9udC1zaXplOiAxLjRyZW07XHJcbiAgbGluZS1oZWlnaHQ6IDEuN3JlbTtcclxuXHJcbiAgLmljb24ge1xyXG4gICAgY3Vyc29yOiBwb2ludGVyO1xyXG4gICAgbWFyZ2luLWxlZnQ6IDEuMnJlbTtcclxuICAgIHdpZHRoOiAxLjdyZW07XHJcbiAgICBoZWlnaHQ6IDEuN3JlbTtcclxuXHJcbiAgICAmLmNvcHkge1xyXG4gICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2NvcHkuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG5cclxuICAgICAgJjpob3ZlciB7XHJcbiAgICAgICAgb3BhY2l0eTogMC43NTtcclxuICAgICAgfVxyXG4gICAgfVxyXG5cclxuICAgICYuY29waWVkIHtcclxuICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9jb21wbGV0ZS10ZXN0d2FsbGV0LnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgIH1cclxuICB9XHJcbn1cclxuXHJcbi5iYWxhbmNlIHtcclxuICBkaXNwbGF5OiBmbGV4O1xyXG4gIGFsaWduLWl0ZW1zOiBmbGV4LWVuZDtcclxuICBqdXN0aWZ5LWNvbnRlbnQ6IGZsZXgtc3RhcnQ7XHJcbiAgZmxleDogMCAwIGF1dG87XHJcbiAgbWFyZ2luOiAyLjZyZW0gMDtcclxuXHJcbiAgOmZpcnN0LWNoaWxkIHtcclxuICAgIGZvbnQtc2l6ZTogMy4zcmVtO1xyXG4gICAgZm9udC13ZWlnaHQ6IDYwMDtcclxuICAgIGxpbmUtaGVpZ2h0OiAyLjRyZW07XHJcbiAgICBtYXJnaW4tcmlnaHQ6IDMuNXJlbTtcclxuICB9XHJcblxyXG4gIDpsYXN0LWNoaWxkIHtcclxuICAgIGZvbnQtc2l6ZTogMS44cmVtO1xyXG4gICAgZm9udC13ZWlnaHQ6IDYwMDtcclxuICAgIGxpbmUtaGVpZ2h0OiAxLjNyZW07XHJcbiAgfVxyXG59XHJcblxyXG4udGFicyB7XHJcbiAgZGlzcGxheTogZmxleDtcclxuICBmbGV4LWRpcmVjdGlvbjogY29sdW1uO1xyXG4gIGZsZXg6IDEgMSBhdXRvO1xyXG5cclxuICAudGFicy1oZWFkZXIge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIGp1c3RpZnktY29udGVudDogc3BhY2UtYmV0d2VlbjtcclxuICAgIGZsZXg6IDAgMCBhdXRvO1xyXG5cclxuICAgIC50YWIge1xyXG4gICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICBhbGlnbi1pdGVtczogY2VudGVyO1xyXG4gICAgICBqdXN0aWZ5LWNvbnRlbnQ6IGNlbnRlcjtcclxuICAgICAgZmxleDogMSAwIGF1dG87XHJcbiAgICAgIGN1cnNvcjogcG9pbnRlcjtcclxuICAgICAgcGFkZGluZzogMCAxcmVtO1xyXG4gICAgICBoZWlnaHQ6IDVyZW07XHJcblxyXG4gICAgICAuaWNvbiB7XHJcbiAgICAgICAgbWFyZ2luLXJpZ2h0OiAxLjNyZW07XHJcbiAgICAgICAgd2lkdGg6IDEuN3JlbTtcclxuICAgICAgICBoZWlnaHQ6IDEuN3JlbTtcclxuXHJcbiAgICAgICAgJi5zZW5kIHtcclxuICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvc2VuZC5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgICAgfVxyXG5cclxuICAgICAgICAmLnJlY2VpdmUge1xyXG4gICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9yZWNlaXZlLnN2Zykgbm8tcmVwZWF0IGNlbnRlcjtcclxuICAgICAgICB9XHJcblxyXG4gICAgICAgICYuaGlzdG9yeSB7XHJcbiAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL2hpc3Rvcnkuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgIH1cclxuXHJcbiAgICAgICAgJi5jb250cmFjdHMge1xyXG4gICAgICAgICAgbWFzazogdXJsKC4uLy4uL2Fzc2V0cy9pY29ucy9jb250cmFjdHMuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgIH1cclxuXHJcbiAgICAgICAgJi5tZXNzYWdlcyB7XHJcbiAgICAgICAgICBtYXNrOiB1cmwoLi4vLi4vYXNzZXRzL2ljb25zL21lc3NhZ2Uuc3ZnKSBuby1yZXBlYXQgY2VudGVyO1xyXG4gICAgICAgIH1cclxuXHJcbiAgICAgICAgJi5zdGFraW5nIHtcclxuICAgICAgICAgIG1hc2s6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvc3Rha2luZy5zdmcpIG5vLXJlcGVhdCBjZW50ZXI7XHJcbiAgICAgICAgfVxyXG4gICAgICB9XHJcblxyXG4gICAgICAuaW5kaWNhdG9yIHtcclxuICAgICAgICBkaXNwbGF5OiBmbGV4O1xyXG4gICAgICAgIGFsaWduLWl0ZW1zOiBjZW50ZXI7XHJcbiAgICAgICAganVzdGlmeS1jb250ZW50OiBjZW50ZXI7XHJcbiAgICAgICAgYm9yZGVyLXJhZGl1czogMXJlbTtcclxuICAgICAgICBmb250LXNpemU6IDFyZW07XHJcbiAgICAgICAgZm9udC13ZWlnaHQ6IDYwMDtcclxuICAgICAgICBtYXJnaW4tbGVmdDogMS4zcmVtO1xyXG4gICAgICAgIHBhZGRpbmc6IDAgMC41cmVtO1xyXG4gICAgICAgIG1pbi13aWR0aDogMS42cmVtO1xyXG4gICAgICAgIGhlaWdodDogMS42cmVtO1xyXG4gICAgICB9XHJcblxyXG4gICAgICAmLmRpc2FibGVkIHtcclxuICAgICAgICBjdXJzb3I6IHVybCguLi8uLi9hc3NldHMvaWNvbnMvbm90LWFsbG93ZWQuc3ZnKSwgbm90LWFsbG93ZWQ7XHJcbiAgICAgIH1cclxuXHJcbiAgICAgICY6bm90KDpsYXN0LWNoaWxkKSB7XHJcbiAgICAgICAgbWFyZ2luLXJpZ2h0OiAwLjNyZW07XHJcbiAgICAgIH1cclxuICAgIH1cclxuICB9XHJcblxyXG4gIC50YWJzLWNvbnRlbnQge1xyXG4gICAgZGlzcGxheTogZmxleDtcclxuICAgIHBhZGRpbmc6IDNyZW07XHJcbiAgICBmbGV4OiAxIDEgYXV0bztcclxuICAgIG92ZXJmbG93LXg6IGhpZGRlbjtcclxuICAgIG92ZXJmbG93LXk6IG92ZXJsYXk7XHJcbiAgfVxyXG59XHJcbiJdfQ== */"

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
        this.queryRouting = this.route.queryParams.subscribe(function (params) {
            if (params.send) {
                _this.tabs.forEach(function (tab, index) {
                    if (tab.link === '/send') {
                        _this.changeTab(index);
                    }
                });
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
    WalletComponent.prototype.closeWallet = function () {
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
    WalletComponent.prototype.ngOnDestroy = function () {
        this.subRouting1.unsubscribe();
        this.subRouting2.unsubscribe();
        this.queryRouting.unsubscribe();
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

module.exports = __webpack_require__(/*! D:\Projects_now\ZANO\zano\src\gui\qt-daemon\html_source\src\main.ts */"./src/main.ts");


/***/ })

},[[0,"runtime","vendor"]]]);
//# sourceMappingURL=main.js.map