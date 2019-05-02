import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { TransferAliasComponent } from './transfer-alias.component';

describe('TransferAliasComponent', () => {
  let component: TransferAliasComponent;
  let fixture: ComponentFixture<TransferAliasComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ TransferAliasComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(TransferAliasComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
