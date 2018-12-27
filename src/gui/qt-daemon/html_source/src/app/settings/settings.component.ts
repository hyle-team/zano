import {Component, OnInit, Renderer2} from '@angular/core';
import {VariablesService} from '../_helpers/services/variables.service';
import {BackendService} from '../_helpers/services/backend.service';
import {FormControl, FormGroup, Validators} from '@angular/forms';
import {Location} from '@angular/common';

@Component({
  selector: 'app-settings',
  templateUrl: './settings.component.html',
  styleUrls: ['./settings.component.scss']
})
export class SettingsComponent implements OnInit {
  theme: string;
  changeForm: any;

  constructor(private renderer: Renderer2, private variablesService: VariablesService, private backend: BackendService, private location: Location) {
    this.theme = this.variablesService.settings.theme;
    this.changeForm = new FormGroup({
      password: new FormControl('', Validators.required),
      new_password: new FormControl('', Validators.required),
      new_confirmation: new FormControl('', Validators.required)
    }, [(g: FormGroup) => {
      return g.get('new_password').value === g.get('new_confirmation').value ? null : {'confirm_mismatch': true};
    }, (g: FormGroup) => {
      return g.get('password').value === this.variablesService.appPass ? null : {'pass_mismatch': true};
    }]);
  }

  ngOnInit() {}

  setTheme(theme) {
    this.renderer.removeClass(document.body, 'theme-' + this.theme);
    this.theme = theme;
    this.variablesService.settings.theme = this.theme;
    this.renderer.addClass(document.body, 'theme-' + this.theme);
    this.backend.storeAppData();
  }

  onSubmitChangePass() {
    if (this.changeForm.valid) {
      this.variablesService.appPass = this.changeForm.get('new_password').value;
      this.backend.storeSecureAppData((status, data) => {
        if (status) {
          this.changeForm.reset();
        } else {
          console.log(data);
        }
      });
    }
  }

  back() {
    this.location.back();
  }

}
